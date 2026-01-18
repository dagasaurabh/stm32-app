#pragma once                                
#include <stdint.h>                         
                                            
/*                                          
 * Monotonic time since boot (milliseconds).
 */                                         
uint64_t time_monotonic_ms(void);           
                                            
/*                                          
 * Monotonic time since boot (microseconds).
 */                                         
uint64_t time_monotonic_us(void);           

