#include <stdio.h>
#include <time.h>

int cmd_vibe_check(int argc, char **argv) {
    printf("bro v1.0.0 vibes check\n");
    printf("=======================\n");
    printf("Feeling good? I sure am.\n");
    printf("It's %s\n", ctime(&(time_t){time(NULL)}));
    printf("Your vibe is: ");
    
    int hour = ((int)time(NULL) / 3600) % 24;
    if (hour >= 6 && hour < 12) {
        printf("MORNING FIRE  Morning coffee energy\n");
    } else if (hour >= 12 && hour < 17) {
        printf("AFTERNOON SLAP  Getting things done\n");
    } else if (hour >= 17 && hour < 21) {
        printf("EVENING DRIP  Wind down mode\n");
    } else {
        printf("NIGHT OWL  Late night coding\n");
    }
    
    return 0;
}
