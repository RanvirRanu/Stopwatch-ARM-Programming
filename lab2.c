/*******************************************************
 * DE10-Standard Stopwatch
 * A9 Private Timer (200 MHz)
 * Time format: MM:SS:HH
 *******************************************************/

#include <stdio.h>

/* Memory Mapped Addresses */
#define MPCORE_PRIV_TIMER 0xFFFEC600
#define KEY_BASE          0xFF200050
#define SW_BASE           0xFF200040
#define HEX3_HEX0_BASE    0xFF200020
#define HEX5_HEX4_BASE    0xFF200030

/* Timer Registers */
volatile int *timer_load   = (int *)(MPCORE_PRIV_TIMER);
volatile int *timer_count  = (int *)(MPCORE_PRIV_TIMER + 0x04);
volatile int *timer_ctrl   = (int *)(MPCORE_PRIV_TIMER + 0x08);
volatile int *timer_status = (int *)(MPCORE_PRIV_TIMER + 0x0C);

/* I/O */
volatile int *KEY_ptr = (int *)KEY_BASE;
volatile int *SW_ptr  = (int *)SW_BASE;
volatile int *HEX3_HEX0_ptr = (int *)HEX3_HEX0_BASE;
volatile int *HEX5_HEX4_ptr = (int *)HEX5_HEX4_BASE;

/* Time Variables */
volatile int current_time = 0;   // in hundredths
volatile int lap_time = 0;
volatile int running = 0;

/* 7-Segment Table */
int hex_table[] = {
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x6F  // 9
};

/*******************************************************
 * Timer Functions
 *******************************************************/

void InitTimer()
{
    *timer_ctrl = 0x0;          // Disable timer
    *timer_load = 2000000;      // 0.01 seconds @ 200 MHz
}

void StartTimer()
{
    *timer_status = 1;          // Clear timeout flag
    *timer_ctrl = 0x3;          // Enable + auto-reload
}

void StopTimer()
{
    *timer_ctrl = 0x0;          // Disable timer
}

int TimerExpired()
{
    if((*timer_status) & 1)
    {
        *timer_status = 1;      // Clear flag
        return 1;
    }
    return 0;
}

/*******************************************************
 * Stopwatch Functions
 *******************************************************/

void ClearTime()
{
    current_time = 0;
    lap_time = 0;
}

void LapTime()
{
    lap_time = current_time;
}

int IntToBinary(int digit)
{
    return hex_table[digit];
}

/*******************************************************
 * Display Function
 *******************************************************/

void DisplayTime(int time)
{
    int hundredths = time % 100;
    int total_seconds = time / 100;

    int seconds = total_seconds % 60;
    int minutes = (total_seconds / 60) % 60;

    int min_tens = minutes / 10;
    int min_ones = minutes % 10;

    int sec_tens = seconds / 10;
    int sec_ones = seconds % 10;

    int hun_tens = hundredths / 10;
    int hun_ones = hundredths % 10;

    int hex3_hex0 =
        (IntToBinary(hun_ones)) |
        (IntToBinary(hun_tens) << 8) |
        (IntToBinary(sec_ones) << 16) |
        (IntToBinary(sec_tens) << 24);

    int hex5_hex4 =
        (IntToBinary(min_ones)) |
        (IntToBinary(min_tens) << 8);

    *HEX3_HEX0_ptr = hex3_hex0;
    *HEX5_HEX4_ptr = hex5_hex4;
}

/*******************************************************
 * Button Handling
 *******************************************************/

void CheckButtons()
{
    int keys = *KEY_ptr;

    if(keys & 0x1)     // KEY0 - Start
    {
        StartTimer();
        running = 1;
    }

    if(keys & 0x2)     // KEY1 - Stop
    {
        StopTimer();
        running = 0;
    }

    if(keys & 0x4)     // KEY2 - Lap
    {
        LapTime();
    }

    if(keys & 0x8)     // KEY3 - Clear
    {
        StopTimer();
        ClearTime();
        running = 0;
    }

    *KEY_ptr = keys;   // Clear edge capture
}

/*******************************************************
 * Main
 *******************************************************/

int main(void)
{
    InitTimer();
    ClearTime();
    DisplayTime(0);

    while(1)
    {
        CheckButtons();

        if(running && TimerExpired())
        {
            current_time++;

            if(current_time >= 360000)   // 60*60*100
                current_time = 0;
        }

        int sw = *SW_ptr & 0x1;

        if(sw == 0)
            DisplayTime(current_time);
        else
            DisplayTime(lap_time);
    }

    return 0;
}
