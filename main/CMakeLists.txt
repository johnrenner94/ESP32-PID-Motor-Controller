idf_component_register(
    SRCS
        "src/main.c"
        "src/motor.c"
        "src/input.c"
        "src/display.c"
        "src/i2cmanager.c"
        "src/calc.c"
        "src/app_tasks.c"
        "src/display_mgr.c"
        "src/ui.c"

    INCLUDE_DIRS "include"
    REQUIRES driver
)