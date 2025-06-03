#include "EnvironmentTags.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Environment_Weather, "Environment.Weather", "Weather");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Environment_Weather_Clear, "Environment.Weather.Clear", "No rain, no snow, may be some clouds casting shadows");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Environment_Weather_Overcast, "Environment.Weather.Overcast", "Overcast, no rain, no snow, completely shadowed");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Environment_Weather_Rain, "Environment.Weather.Rain", "Rain, its raining, and its overcast. High winds = storm");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Environment_Weather_Snow, "Environment.Weather.Snow", "Snow, its snowing, and its overcast. High winds = blizzard");
