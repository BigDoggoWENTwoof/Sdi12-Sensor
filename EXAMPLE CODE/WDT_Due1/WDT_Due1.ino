#define WDT_KEY (0xA5)
/********************************************************************************
  extern "C" void _watchdogDefaultSetup (void) { WDT_Disable (WDT); }

  void watchdogSetup (void) __attribute__ ((weak, alias("_watchdogDefaultSetup")));
*********************************************************************************/

/*This function is called from init(). If the user does not provide
  this function, then the default action is to disable watchdog.
  This function has to be overriden, otherwise watchdog won't work !! */

void watchdogSetup(void) {
  /*** watchdogDisable (); ***/
}

//void watchdogDefaultSetup (void) { }

//void watchdogSetup(void) { watchdogDefaultSetup ();}

void setup()
{
  // Enable watchdog.
  WDT->WDT_MR = WDT_MR_WDD(0xFFF) |
                WDT_MR_WDFIEN |  //  Triggers an interrupt or WDT_MR_WDRSTEN to trigger a Reset
                WDT_MR_WDV(256 * 2); // Watchdog triggers a reset or an interrupt after 2 seconds if underflow
  // 2 seconds equal 84000000 * 2 = 168000000 clock cycles
  /* Slow clock is running at 32.768 kHz
    watchdog frequency is therefore 32768 / 128 = 256 Hz
    WDV holds the periode in 256 th of seconds  */
  NVIC_EnableIRQ(WDT_IRQn);

  Serial.begin(9600);
  uint32_t status = (RSTC->RSTC_SR & RSTC_SR_RSTTYP_Msk) >> RSTC_SR_RSTTYP_Pos /*8*/; // Get status from the last Reset
  Serial.print("RSTTYP = 0b"); Serial.println(status, BIN);  // Should be 0b010 after first watchdog reset

}

void loop()
{

  //Restart watchdog
  WDT->WDT_CR = WDT_CR_KEY(WDT_KEY)
                | WDT_CR_WDRSTT;

  Serial.println("Enter the main loop : Restart watchdog");
  GPBR->SYS_GPBR[0] += 1;
  Serial.print("GPBR = "); Serial.println(GPBR->SYS_GPBR[0]);
  delay(500);

  while (true)
  {
    Serial.println("the software becomes trapped in a deadlock !");
    delay(500);
    /* If the software becomes trapped in a deadlock,
       watchdog triggers a reset or an interrupt. if there is a Reset, the software restarts with stored values
       in General Purpose Back up Registers*/
  }
}
void WDT_Handler(void)
{
  /* Clear status bit to acknowledge interrupt by dummy read. */
  WDT->WDT_SR; // Clear status register

  printf("help! in WDT\n");
}