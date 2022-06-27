#pragma once
#define BPCVER_MAJOR 0
#define BPCVER_MINOR 1
#define BPCVER_BUILD 0

#define BPCVER_NUM 1

#define BPCVER_TOSTRING2(arg) #arg
#define BPCVER_TOSTRING(arg) BPCVER_TOSTRING2(arg)
#define BPCVER_VERSION BPCVER_TOSTRING(BPCVER_MAJOR) "." BPCVER_TOSTRING(BPCVER_MINOR) "." BPCVER_TOSTRING(BPCVER_BUILD)
#define BPCVER_RC_VERSION BPCVER_TOSTRING(BPCVER_MAJOR) "." BPCVER_TOSTRING(BPCVER_MINOR) "." BPCVER_TOSTRING(BPCVER_BUILD) ".0"