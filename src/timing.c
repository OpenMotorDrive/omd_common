/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common/timing.h>
#include <ch.h>

#include <common/can.h>

static thread_t* worker_thread;
static THD_WORKING_AREA(waTimingThread, 128);
static THD_FUNCTION(TimingThread, arg);

static struct {
    uint32_t update_seconds;
    systime_t update_systime;
} timing_state[2];

static volatile uint8_t timing_state_idx;

void timing_init(void)
{
    if (!worker_thread) {
        worker_thread = chThdCreateStatic(waTimingThread, sizeof(waTimingThread), HIGHPRIO-1, TimingThread, NULL);
    }
}

uint32_t millis(void) {
    uint8_t idx = timing_state_idx;
    systime_t systime_now = chVTGetSystemTimeX();
    uint32_t delta_ticks = systime_now-timing_state[idx].update_systime;
    uint32_t delta_ms = delta_ticks / (CH_CFG_ST_FREQUENCY/1000);
    return (timing_state[idx].update_seconds*1000) + delta_ms;
}

uint32_t micros(void) {
    uint8_t idx = timing_state_idx;
    systime_t systime_now = chVTGetSystemTimeX();
    uint32_t delta_ticks = systime_now-timing_state[idx].update_systime;
    uint32_t delta_ms = delta_ticks / (CH_CFG_ST_FREQUENCY/1000000);
    return (timing_state[idx].update_seconds*1000000) + delta_ms;
}

void usleep(uint32_t delay) {
    uint32_t tbegin = micros();
    while (micros()-tbegin < delay);
}

static THD_FUNCTION(TimingThread, arg)
{
    (void)arg;
    while (true) {
        uint8_t next_timing_state_idx = (timing_state_idx+1) % 2;

        systime_t systime_now = chVTGetSystemTimeX();
        uint32_t delta_ticks = systime_now-timing_state[timing_state_idx].update_systime;

        timing_state[next_timing_state_idx].update_seconds = timing_state[timing_state_idx].update_seconds + delta_ticks / CH_CFG_ST_FREQUENCY;
        timing_state[next_timing_state_idx].update_systime = systime_now - (delta_ticks % CH_CFG_ST_FREQUENCY);

        timing_state_idx = next_timing_state_idx;

        chThdSleepSeconds(10);
    }
}
