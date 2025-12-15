#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

typedef enum {
    MODE_IDLE = 0,
    MODE_200M = 1,
    MODE_400M = 2,
    MODE_800M = 3
} running_mode_t;

//hearth rate mesaurement
//innt measure hearthrate

void display_hearth_rate(int bmp){
    printf("heart rate:" , bpm);

}

running_mode_t get_selected_mode() {
    printf("\nSelect Mode:\n");
    printf("1 - 200m Sprint\n");
    printf("2 - 400m Run\n");
    printf("3 - 800m Run\n");
    printf("Enter choice: ");
    
    int choice;
    scanf("%d", &choice);
    return (running_mode_t)choice;
}


void wait_for_start_button() {
    printf("Press ENTER when ready to start...");
    getchar();
    getchar();  // Clear buffer
}

void wait_for_finish_button() {
    printf("Press ENTER when finished running...");
    getchar();
}

void run_mode(running_mode_t mode) {
    const char* mode_names[] = {"IDLE", "200m Sprint", "400m Run", "800m Run"};
    
    printf("\n🏃 Starting %s Mode\n", mode_names[mode]);
    printf("==================\n");
    
    // Pre-run heart rate
    printf("📏 Measuring resting heart rate...\n");
    int pre_hr = measure_heart_rate();
    display_heart_rate(pre_hr);
    
    // Start sequence
    wait_for_start_button();
    printf("🚀 GO! Start running!\n");
    run_buzzer();  // Signal start
    
    // Wait for completion
    wait_for_finish_button();
    printf("🏁 Finished! Measuring recovery heart rate...\n");
    
    // Post-run heart rate
    int post_hr = measure_heart_rate();
    display_heart_rate(post_hr);
    
    // Show results
    printf("\n📊 Results for %s:\n", mode_names[mode]);
    printf("   Resting HR: %d BPM\n", pre_hr);
    printf("   Recovery HR: %d BPM\n", post_hr);
    printf("   HR Increase: %d BPM\n", post_hr - pre_hr);
    printf("==================\n");
}

// Main running modes controller
void start_running_modes() {
    srand(time(NULL));  // For random heart rate simulation
    
    while(1) {
        running_mode_t mode = get_selected_mode();
        
        if (mode == MODE_IDLE) {
            printf("👋 Exiting running modes\n");
            break;
        }
        
        if (mode >= MODE_200M && mode <= MODE_800M) {
            run_mode(mode);
        } else {
            printf("❌ Invalid mode selected\n");
        }
    }
}