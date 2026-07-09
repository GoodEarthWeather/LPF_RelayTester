#include "driverlib.h"
#include "main.h"


// Define all I/O


// Define the GPIO Pin used for the 74HCT595 Latch Signal (RCLK)
#define LATCH_PORT   GPIO_PORT_P1
#define LATCH_PIN    GPIO_PIN7

// Track valid button press states globally
volatile uint8_t button_pressed_flag = 0;

#define delay_ms(x)     __delay_cycles((long) x* 1000 * 8)
#define delay_us(x)     __delay_cycles((long) x * 8)

void initClocks(void);
void init_spi_shift_register(void);
void initGPIO(void);
void send_byte_to_shift_register(uint8_t);
void init_switch_s1(void);

// Define the GPIO Pin used for the 74HCT595 Latch Signal (RCLK)
#define LATCH_PORT   GPIO_PORT_P1
#define LATCH_PIN    GPIO_PIN7



int main(void) {

    WDT_A_hold(WDT_A_BASE);
    initGPIO();
    initClocks();
    WDT_A_hold(WDT_A_BASE); // Halt Watchdog Timer

    init_spi_shift_register(); // Hardware setup from the previous step
    init_switch_s1();          // S1 and Timer_A3 debounce setup

    uint8_t data_to_send = SELECT_40M;

    while (1)
    {
        if (button_pressed_flag)
        {
            button_pressed_flag = 0; // Reset event flag

            // Execute task: Send shifted data byte over eUSCI_B0
            send_byte_to_shift_register(data_to_send);
            delay_ms(12);
            send_byte_to_shift_register(0);

            // Increment or modify the test byte
            if ( data_to_send == SELECT_40M) {
                data_to_send = SELECT_2030M;
            } else if (data_to_send == SELECT_2030M) {
                data_to_send = SELECT_1517M;
            } else {
                data_to_send = SELECT_40M;
            }
        }
    }
}



// Routine from Gemini to shift data to the 595
//#include "driverlib.h"


void init_spi_shift_register(void)
{
    // 1. Configure Hardware SPI Pins for eUSCI_B0
    // P1.1 = UCB0CLK (Clock Out)
    // P1.2 = UCB0SIMO (Data Out)
    // GPIO_PRIMARY_MODULE_FUNCTION activates the internal eUSCI hardware multiplexer
    GPIO_setAsPeripheralModuleFunctionOutputPin(
        GPIO_PORT_P1,
        GPIO_PIN1 | GPIO_PIN2,
        GPIO_PRIMARY_MODULE_FUNCTION
    );

    // 2. Configure Latch Pin (P1.7) as a standard GPIO Output
    GPIO_setAsOutputPin(LATCH_PORT, LATCH_PIN);
    GPIO_setOutputLowOnPin(LATCH_PORT, LATCH_PIN);

    // 3. Initialize eUSCI_B0 SPI Master Configuration Structure
    EUSCI_B_SPI_initMasterParam param = {0};
    param.selectClockSource = EUSCI_B_SPI_CLOCKSOURCE_SMCLK;             // Use Sub-Main Clock
    param.clockSourceFrequency = CS_getSMCLK();                          // Grab current SMCLK frequency automatically
    param.desiredSpiClock = 1000000;                                     // Drive transmission at 1 MHz
    param.msbFirst = EUSCI_B_SPI_MSB_FIRST;                              // 74HCT595 shifts in MSB first
    param.clockPolarity = EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_LOW;      // CPOL = 0 (Clock low when idle)
    param.clockPhase = EUSCI_B_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT; // CPHA = 0 (Data changes on falling, latched on rising edge)
    param.spiMode = EUSCI_B_SPI_3PIN;                                    // 3-Wire Mode (Clock, SIMO, ignoring SOMI line)

    // 4. Initialize and Enable the eUSCI_B0 Peripheral Module
    EUSCI_B_SPI_initMaster(EUSCI_B0_BASE, &param);
    EUSCI_B_SPI_enable(EUSCI_B0_BASE);

    // Clear residual eUSCI_B0 RX flag interrupts before starting
    EUSCI_B_SPI_clearInterrupt(EUSCI_B0_BASE, EUSCI_B_SPI_RECEIVE_INTERRUPT);

    // Disable FRAM power/IO lock (Critical step for MSP430 FR2xx line to activate external pins)
    // comment out below - this is done in the initGPIO() routine
    //PMM_unlockLPM5();
}

void send_byte_to_shift_register(uint8_t data)
{
    // Wait until the hardware transmit buffer is empty and ready
    while (!EUSCI_B_SPI_getInterruptStatus(EUSCI_B0_BASE, EUSCI_B_SPI_TRANSMIT_INTERRUPT));

    // Send the byte into the hardware buffer (it will auto-shift out 8 bits in the background)
    EUSCI_B_SPI_transmitData(EUSCI_B0_BASE, data);

    // Block until the shift register inside the eUSCI_B0 completely finishes serialization
    while (EUSCI_B_SPI_isBusy(EUSCI_B0_BASE));

    // Pulse the P1.7 Latch pin (RCLK) to push data to the 74HCT595 parallel physical pins
    GPIO_setOutputHighOnPin(LATCH_PORT, LATCH_PIN);
    __delay_cycles(10); // Small delay to satisfy 74HCT595 timing minimums
    GPIO_setOutputLowOnPin(LATCH_PORT, LATCH_PIN);
}

//#include "driverlib.h"


void init_switch_s1(void)
{
    // 1. Configure S1 (P4.0) as Input with an Internal Pull-Up Resistor
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN0);

    // 2. Configure P4.0 Interrupts: Trigger on a High-to-Low transition (falling edge)
    GPIO_selectInterruptEdge(GPIO_PORT_P4, GPIO_PIN0, GPIO_HIGH_TO_LOW_TRANSITION);
    GPIO_clearInterrupt(GPIO_PORT_P4, GPIO_PIN0);
    GPIO_enableInterrupt(GPIO_PORT_P4, GPIO_PIN0);

    // 3. Configure Timer_A3 for the Debounce Timeout Period
    // Using SMCLK (assumed ~1MHz) in Up Mode. 15,000 counts at 1MHz = 15ms delay.
    Timer_A_initUpModeParam timerParam = {0};
    timerParam.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    timerParam.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    timerParam.timerPeriod = 15000;
    timerParam.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
    timerParam.captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE; // Enable CCR0 Interrupt
    timerParam.timerClear = TIMER_A_DO_CLEAR;
    timerParam.startTimer = false; // Do not start running yet

    Timer_A_initUpMode(TIMER_A3_BASE, &timerParam);

    // Disable FRAM power/IO lock (Required step for MSP430 FR2xx line)
    PMM_unlockLPM5();

    // Enable Global Interrupts (GIE bit in Status Register)
    __enable_interrupt();
}

// -------------------------------------------------------------------------
// PORT 4 INTERRUPT SERVICE ROUTINE (Initial Button Edge Detection)
// -------------------------------------------------------------------------
#pragma vector=PORT4_VECTOR
__interrupt void Port_4_ISR(void)
{
    // Check if the interrupt was caused by the P4.0 switch pin
    if (GPIO_getInterruptStatus(GPIO_PORT_P4, GPIO_PIN0) == GPIO_PIN0)
    {
        // Temporarily disable the P4.0 interrupt to shield against mechanical bounce noise
        GPIO_disableInterrupt(GPIO_PORT_P4, GPIO_PIN0);
        GPIO_clearInterrupt(GPIO_PORT_P4, GPIO_PIN0);

        // Clear and restart Timer_A3 to begin the 15ms verification window
        Timer_A_clearTimerInterrupt(TIMER_A3_BASE);
        Timer_A_startCounter(TIMER_A3_BASE, TIMER_A_UP_MODE);
    }
}

// -------------------------------------------------------------------------
// TIMER_A3 CCR0 INTERRUPT SERVICE ROUTINE (Debounce Verification)
// -------------------------------------------------------------------------
#pragma vector=TIMER3_A0_VECTOR
__interrupt void Timer_A3_CCR0_ISR(void)
{
    // Halt Timer_A3 since our verification check is happening now
    Timer_A_stop(TIMER_A3_BASE);
    Timer_A_clear(TIMER_A3_BASE);

    // Sample the pin. If it remains LOW (0), it is a stable, intended press.
    if (GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN0) == GPIO_INPUT_PIN_LOW)
    {
        button_pressed_flag = 1; // Mark event for main program loop
    }

    // Clear residual bounce noise, then re-arm the pin edge interrupt
    GPIO_clearInterrupt(GPIO_PORT_P4, GPIO_PIN0);
    GPIO_enableInterrupt(GPIO_PORT_P4, GPIO_PIN0);
}

