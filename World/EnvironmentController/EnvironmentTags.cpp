#include "EnvironmentTags.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Environment_Weather, "Environment.Weather", "Weather");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Environment_Weather_Clear, "Environment.Weather.Clear", "No rain, no snow, may be some clouds casting shadows");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Environment_Weather_Overcast, "Environment.Weather.Overcast", "Overcast, no rain, no snow, completely shadowed");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Environment_Weather_Rain, "Environment.Weather.Rain", "Rain, its raining, and its overcast. High winds = storm");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Environment_Weather_Snow, "Environment.Weather.Snow", "Snow, its snowing, and its overcast. High winds = blizzard");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Environment_Time, "Environment.Time", "Time of day (mirrors EDayTime)");
// Hour windows MUST mirror HourAsDayTime() (MythicEnvironmentController.h) — GetDayTimeTag() derives the tag straight
// from EDayTime, so these annotations are the actual gameplay-query windows a designer gates on. Morning starts at 07:00
// (not 06:00); see BACKLOG for the open design question of aligning this with the schedule's 06:00 day-start.
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Environment_Time_Morning, "Environment.Time.Morning", "07:00 - 12:00");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Environment_Time_Afternoon, "Environment.Time.Afternoon", "12:00 - 17:00");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Environment_Time_Evening, "Environment.Time.Evening", "17:00 - 20:00");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Environment_Time_Night, "Environment.Time.Night", "20:00 - 07:00");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Environment_Season, "Environment.Season", "Season of year (mirrors ESeason)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Environment_Season_Spring, "Environment.Season.Spring", "Months 3, 4, 5");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Environment_Season_Summer, "Environment.Season.Summer", "Months 6, 7, 8");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Environment_Season_Autumn, "Environment.Season.Autumn", "Months 9, 10, 11");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Environment_Season_Winter, "Environment.Season.Winter", "Months 12, 1, 2");
