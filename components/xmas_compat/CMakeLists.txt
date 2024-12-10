idf_component_register(SRCS "gpio_compat.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver mcp23x17 pcf8575)
