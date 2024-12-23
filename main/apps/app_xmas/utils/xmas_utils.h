#include "../../../hal/hal.h"
#include <mcp23x17.h>

namespace MOONCAKE
{
    namespace USER_APP
    {
        namespace XMAS
        {
            class Utils {
                public:
                    static void drawCenterString(HAL::HAL* hal, const char *string);
                    static bool checkButton(mcp23x17_t* dev, int pin_button);
            };
        }
    }
}