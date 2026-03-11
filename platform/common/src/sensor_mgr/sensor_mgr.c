#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <assert.h>
#include "board.h"
#include "sensor_mgr.h"
#include "systime.h"

#define SENSOR_MGR_TIMEOUT_MS 500
#define SENSOR_MGR_MAX_ERRORS 3

#ifdef BOARD_HAS_SENSORS

typedef struct
{
    sensor_data_cb_t cb;
    void *ctx;
} subscriber_t;

typedef struct
{
    int handle; /* own slot index — passed back in callbacks */
    sensor_id_t type;
    sensor_t *device;
    uint32_t interval_ms;
    uint32_t arm_time_ms;
    bool streaming;
    bool read_pending;
    int error_count;
    subscriber_t *subs; /* slice into s_mgr.subs[] — set at registration */
    uint8_t n_subs;     /* allocated count */
    uint8_t sub_count;  /* currently registered */
    /*
     * data_buf — persistent storage for the most recent sensor reading.
     *
     * sensor_get_data() writes here from ISR context (_on_done).  Keeping it
     * in the entry (rather than on the stack of _on_done) ensures the pointer
     * passed to subscriber callbacks remains valid for the duration of the
     * callback chain.  Size: SENSOR_MAX_DATA_SIZE bytes per entry.
     * Total .bss cost: BOARD_MAX_SENSORS * SENSOR_MAX_DATA_SIZE bytes.
     */
    uint8_t data_buf[SENSOR_MAX_DATA_SIZE];
} sensor_entry_t;

struct _sensor_manager
{
    sensor_entry_t sensor[BOARD_MAX_SENSORS];
    subscriber_t subs[BOARD_MAX_SUBS];
    uint8_t subs_used;
    uint8_t sensor_count;
};

static sensor_manager_t s_mgr;

/* ------------------------------------------------------------------ */
/* Internal helpers                                                     */
/* ------------------------------------------------------------------ */

static sensor_entry_t *
entry_from_handle(int handle)
{
    if (handle < 0 || handle >= s_mgr.sensor_count)
        return NULL;
    return &s_mgr.sensor[handle];
}

static void
arm_read(sensor_entry_t *e);

static void
_on_done(sensor_t *self, int status, void *ctx)
{
    sensor_entry_t *e = (sensor_entry_t *)ctx;
    e->read_pending = false;

    if (status == 0)
    {
        sensor_get_data(self, e->data_buf);
        e->error_count = 0;
    }
    else
    {
        if (++e->error_count >= SENSOR_MGR_MAX_ERRORS)
            e->streaming = false;
    }

    for (int i = 0; i < e->sub_count; i++)
        e->subs[i].cb(e->handle, status == 0 ? (const void *)e->data_buf : NULL, status,
                      e->subs[i].ctx);
}

static void
arm_read(sensor_entry_t *e)
{
    e->read_pending = true;
    e->arm_time_ms = time_monotonic_ms();

    if (sensor_read_async(e->device, _on_done, e) != 0)
        e->read_pending = false;
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

int
sensor_mgr_register(sensor_id_t type, sensor_t *device, uint32_t interval_ms, uint8_t n_subs)
{
    if (!device)
        return -EINVAL;
    if (s_mgr.sensor_count >= BOARD_MAX_SENSORS)
        return -ENOMEM;
    if (s_mgr.subs_used + n_subs > BOARD_MAX_SUBS)
        return -ENOMEM;

    /* Catch data_size mismatch at startup — programming error if triggered.
     * Increase SENSOR_MAX_DATA_SIZE in sensor_mgr.h to fix. */
    assert(device->data_size > 0 && device->data_size <= SENSOR_MAX_DATA_SIZE);

    int handle = s_mgr.sensor_count++;
    sensor_entry_t *e = &s_mgr.sensor[handle];
    e->handle = handle;
    e->type = type;
    e->device = device;
    e->interval_ms = interval_ms;
    e->subs = &s_mgr.subs[s_mgr.subs_used];
    e->n_subs = n_subs;
    s_mgr.subs_used += n_subs;
    return handle;
}

int
sensor_mgr_subscribe(int handle, sensor_data_cb_t cb, void *ctx)
{
    if (!cb)
        return -EINVAL;
    sensor_entry_t *e = entry_from_handle(handle);
    if (!e)
        return -ENODEV;
    if (e->sub_count >= e->n_subs)
        return -ENOMEM;
    e->subs[e->sub_count].cb = cb;
    e->subs[e->sub_count].ctx = ctx;
    e->sub_count++;
    return 0;
}

int
sensor_mgr_unsubscribe(int handle, sensor_data_cb_t cb)
{
    sensor_entry_t *e = entry_from_handle(handle);

    if (!e)
        return -ENODEV;

    for (int i = 0; i < e->sub_count; i++)
    {
        if (e->subs[i].cb == cb)
        {
            e->subs[i] = e->subs[e->sub_count - 1];
            e->subs[e->sub_count - 1].cb = NULL;
            e->sub_count--;
            return 0;
        }
    }
    return -ENOENT;
}

int
sensor_mgr_read_sync(int handle, void *out)
{
    if (!out)
        return -EINVAL;

    sensor_entry_t *e = entry_from_handle(handle);

    if (!e || !e->device)
        return -ENODEV;

    return sensor_read_sync(e->device, out);
}

int
sensor_mgr_read(int handle)
{
    sensor_entry_t *e = entry_from_handle(handle);

    if (!e || !e->device)
        return -ENODEV;

    if (e->read_pending)
        return -EBUSY;

    arm_read(e);
    return 0;
}

int
sensor_mgr_stream_start(int handle)
{
    sensor_entry_t *e = entry_from_handle(handle);

    if (!e || !e->device)
        return -ENODEV;

    e->streaming = true;

    if (!e->read_pending)
        arm_read(e);
    return 0;
}

int
sensor_mgr_stream_stop(int handle)
{
    sensor_entry_t *e = entry_from_handle(handle);
    if (!e)
        return -ENODEV;
    e->streaming = false;
    return 0;
}

int
sensor_mgr_reset(int handle)
{
    sensor_entry_t *e = entry_from_handle(handle);
    if (!e)
        return -ENODEV;
    e->error_count = 0;
    e->read_pending = false;
    e->streaming = false; /* caller must call sensor_mgr_stream_start() to resume */
    return 0;
}

void
sensor_mgr_poll(void)
{
    uint32_t now = time_monotonic_ms();

    for (int i = 0; i < s_mgr.sensor_count; i++)
    {
        sensor_entry_t *e = &s_mgr.sensor[i];

        /* timeout watchdog — fires when _on_done is never called (bus hang) */
        if (e->read_pending && (now - e->arm_time_ms) > SENSOR_MGR_TIMEOUT_MS)
        {

            e->read_pending = false;
            if (++e->error_count >= SENSOR_MGR_MAX_ERRORS)
                e->streaming = false;

            for (int j = 0; j < e->sub_count; j++)
                e->subs[j].cb(e->handle, NULL, -ETIMEDOUT, e->subs[j].ctx);

            continue;
        }

        /* streaming re-arm */
        if (e->streaming && !e->read_pending && (now - e->arm_time_ms) >= e->interval_ms)
            arm_read(e);
    }
}

#else /* !BOARD_HAS_SENSORS */

int
sensor_mgr_register(sensor_id_t t, sensor_t *d, uint32_t ms, uint8_t n)
{
    (void)t;
    (void)d;
    (void)ms;
    (void)n;
    return -ENODEV;
}
int
sensor_mgr_subscribe(int h, sensor_data_cb_t cb, void *ctx)
{
    (void)h;
    (void)cb;
    (void)ctx;
    return -ENODEV;
}
int
sensor_mgr_unsubscribe(int h, sensor_data_cb_t cb)
{
    (void)h;
    (void)cb;
    return -ENODEV;
}
int
sensor_mgr_read_sync(int h, void *out)
{
    (void)h;
    (void)out;
    return -ENODEV;
}
int
sensor_mgr_read(int h)
{
    (void)h;
    return -ENODEV;
}
int
sensor_mgr_stream_start(int h)
{
    (void)h;
    return -ENODEV;
}
int
sensor_mgr_stream_stop(int h)
{
    (void)h;
    return -ENODEV;
}
int
sensor_mgr_reset(int h)
{
    (void)h;
    return -ENODEV;
}
void
sensor_mgr_poll(void)
{
}

#endif /* BOARD_HAS_SENSORS */
