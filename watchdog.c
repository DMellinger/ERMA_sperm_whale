
#include "erma.h"

/***************************** Configuration ***************************/
float
    missionDurDays	= 35,	//mission duration, days
    rpiPower		= 5.0,	//power draw of Raspberry Pi, watts
    gliderWisprPower	= 2.0,	//power draw of glider+WISPR, watts
    batteryEnergy	= 17e6,	//battery capacity, joules
    reserveBattCapacity	= 0.35,	//reserve battery: fraction we want left at end
    avgDiveTimeHr	= 4.5,	//average length of a dive, hours
    rpiRunTimeLimitMin	= 30;	//max time we want RPi to run per dive, minutes
/************************** End of Configuration ************************/


int main(int argc, char **argv)
{
    //Using the config parameters above, calculate how many seconds we could run
    //the RPI for on each run (there are 2 runs per dive cycle, during ascent
    //and descent).
    float
	availEnergy = batteryEnergy * (1 - reserveBattCapacity),//joules
        missionDurS = missionDurDays * 24*60*60,		//seconds
        rpiRunTimeLimitS = rpiRunTimeLimitMin * 60,		//seconds
	gliderWisprEnergy = gliderWisprPower * missionDurS,	//joules
        rpiAvailEnergy = availEnergy - gliderWisprEnergy,	//joules
	rpiAvailSec = rpiAvailEnergy / rpiPower,//total time we could run RPi
        avgDiveTimeS = avgDiveTimeHr * 60*60,	//seconds
        nDives = missionDurS / avgDiveTimeS,	//number of dives in mission
	nRpiRunsPerDive = 2,			//descent + ascent
	//time avail per dive to run RPi based on energy availability:
	maxRpiSecPerRun = rpiAvailSec / nDives / nRpiRunsPerDive,
	rpiSecPerRun = fmin(maxRpiSecPerRun, rpiRunTimeLimitS);
	
    printf("watchdog: sleeping for %d seconds (have enough power for %d)\n",
	   (long)rpiSecPerRun, (long)maxRpiSecPerRun);
    sleep((long)rpiSecPerRun);
#if ON_RPI
    if (gpio_export(RPI_ACTIVE_GPIO_PIN) != 0) {//make the pin accessible
	printf("watchdog: Unable to export GPIO pin %d! Quitting.\n",
	       RPI_ACTIVE_GPIO_PIN);
    } else {
	gpio_set_direction(RPI_ACTIVE_GPIO_PIN, 1);   //say pin is for output
	gpio_set_value(RPI_ACTIVE_GPIO_PIN, 0);	//tell WISPR to shut off power
	system("sudo shutdown -h now");
    }
#endif

    return 0;
}
