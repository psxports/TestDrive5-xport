#include <string.h>

#include "records.h"
#include "game.h"

typedef char RECORD_DATA_SIZE_CHECK[sizeof(RECORD_DATA) == 20 ? 1 : -1];

typedef char RECORD_ENTRY_SIZE_CHECK[sizeof(RECORD_ENTRY) == 20 ? 1 : -1];

typedef char RECORD_TABLE_SIZE_CHECK[sizeof(RECORD_TABLE) == 5640 ? 1 : -1];

static const uint32 initial_vals[47] = {8550, 6600, 7800, 6150, 6000, 6000, 8550, 7200, 7800, 6150, 6000, 6000, 8100, 6000, 7500, 7200, 8100, 8100, 6300, 7500, 7200, 7950, 7800, 7800, 3000, 3300, 3000, 2700, 3000, 13300, 13300, 2100, 2100, 2100, 2100, 2100, 2700, 2700, 3000, 3000, 30000, 37800, 45600, 63000, 68400, 72000, 0};
static const uint32 model_speeds[16] = {904, 904, 1066, 885, 841, 841, 841, 841, 841, 841, 914, 855, 1002, 899, 899, 1046};
static const uint32 rec_weights[47] = {3175, 2642, 3362, 2648, 2880, 3000, 3175, 2642, 3362, 2648, 2880, 3000, 2574, 2658, 2656, 2684, 2988, 2610, 2635, 2657, 2684, 2989, 2649, 2668, 683, 729, 600, 542, 634, 300, 1, 1, 1, 1, 1, 1, 709, 1, 616, 1, 10939, 16513, 21803, 28340, 33876, 15216, 1};

RECORD_TABLE rec_table = {.words = {
                              1024u, 1163154176u, 1313163344u, 2080374784u, 195u, 1224u, 1447381761u, 20041u,      2080505856u, 195u, 2024u, 1481984000u, 5850444u,    2080636928u, 195u, 1424u, 1380008960u, 5129554u,    2583953408u, 195u, 1064u, 1230128386u, 67u,         2416312320u, 195u, 1024u, 1380008960u, 5129554u, 2080768000u, 195u, 1024u, 1128879616u, 1230131028u, 2080374849u, 195u,     1224u, 1296648705u, 22861u,      2080505856u, 195u,     1024u, 1162363648u, 1095062348u, 2080636928u, 195u,     1424u, 1213155840u, 78u,         2583953408u, 195u,     1624u, 1380467458u, 21321u,      2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,   1024u, 1128878592u, 1146241352u, 2080374784u, 195u,     1224u, 1497451009u, 0u,          2080505856u, 195u,     1024u, 1263095040u, 69u,         2080636928u, 195u,     1424u, 1262836224u, 69u,         2583953408u, 195u,     1624u, 1380009730u, 4740165u,    2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,
                              1024u, 1312901376u, 0u,          2080374784u, 195u, 1224u, 1145656065u, 5850446u,    2080505856u, 195u, 1024u, 1162368000u, 4279628u,    2080636928u, 195u, 1424u, 1095910912u, 19278u,      2583953408u, 195u, 1624u, 1414676738u, 5395784u,    2416312320u, 195u, 0u,    0u,          0u,       0u,          0u,   1024u, 1314475008u, 5132100u,    2080374784u, 195u,     1224u, 1279345409u, 5130582u,    2080505856u, 195u,     1024u, 1330599680u, 1464816196u, 2080636928u, 195u,     1424u, 1380272128u, 1414677826u, 2583953408u, 195u,     1624u, 1380009986u, 22866u,      2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,   1024u, 1413565696u, 84u,         2080374784u, 195u,     1224u, 1229734145u, 5132115u,    2080505856u, 195u,     1024u, 1329941504u, 1146241358u, 2080636928u, 195u,     1424u, 1430343680u, 76u,         2583953408u, 195u,     1624u, 1128356354u, 1498562888u, 2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,
                              1024u, 1313819904u, 22868u,      2080374784u, 195u, 1224u, 1413565185u, 69u,         2080505856u, 195u, 1024u, 1380273664u, 79u,         2080636928u, 195u, 1424u, 1397637376u, 19525u,      2583953408u, 195u, 1624u, 1431258114u, 1396788295u, 2416312320u, 195u, 0u,    0u,          0u,       0u,          0u,   1024u, 1430342656u, 4998482u,    2080374784u, 195u,     1224u, 1163281665u, 0u,          2080505856u, 195u,     1024u, 1413825024u, 22868u,      2080636928u, 195u,     1424u, 1313427968u, 20295u,      2583953408u, 195u,     1624u, 1145324802u, 0u,          2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,   1024u, 1229278720u, 76u,         2080374784u, 195u,     1224u, 1515541249u, 1162759745u, 2080505856u, 195u,     1024u, 1380930048u, 89u,         2080636928u, 195u,     1424u, 1230129920u, 1313428563u, 2583953408u, 195u,     1624u, 1330398722u, 1129203026u, 2416312389u, 195u,     0u,    0u,          0u,       0u,          0u,
                              1024u, 1398096128u, 5130580u,    2080374784u, 195u, 1224u, 1380011265u, 89u,         2080505856u, 195u, 1024u, 1095254784u, 1397050450u, 2080636928u, 195u, 1424u, 1431258112u, 4997447u,    2583953408u, 195u, 1624u, 1162828290u, 0u,          2416312320u, 195u, 0u,    0u,          0u,       0u,          0u,   1024u, 1313751296u, 1279607361u, 2080374784u, 195u,     1224u, 1414873601u, 18499u,      2080505856u, 195u,     1024u, 1229014016u, 22867u,      2080636928u, 195u,     1424u, 1314212352u, 69u,         2583953408u, 195u,     1624u, 1314210818u, 5128515u,    2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,   1024u, 1431589632u, 5526081u,    2080374784u, 195u,     1224u, 1313751297u, 69u,         2080505856u, 195u,     1024u, 1280657920u, 16713u,      2080636928u, 195u,     1424u, 1179996672u, 21333u,      2583953408u, 195u,     1624u, 1229475842u, 76u,         2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,
                              1024u, 1279872000u, 89u,         2080374784u, 195u, 1224u, 1312900097u, 22862u,      2080505856u, 195u, 1024u, 1163219200u, 1313428558u, 2080636928u, 195u, 1424u, 1398096896u, 5130580u,    2583953408u, 195u, 1624u, 1280657922u, 17737u,      2416312320u, 195u, 0u,    0u,          0u,       0u,          0u,   1024u, 1230262016u, 83u,         2080374784u, 195u,     1224u, 1430799105u, 19267u,      2080505856u, 195u,     1024u, 1163018496u, 4278356u,    2080636928u, 195u,     1424u, 1464616192u, 4476751u,    2583953408u, 195u,     1624u, 1279874818u, 5132115u,    2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,   1024u, 1095715328u, 1296648259u, 2080374784u, 4194499u, 1224u, 1397705217u, 4739141u,    2080505856u, 2097347u, 1024u, 1263881216u, 69u,         2080636928u, 5701827u, 1424u, 1094992128u, 77u,         2583953408u, 4194499u, 1624u, 1380011266u, 75u,         2416312320u, 1441987u, 0u,    0u,          0u,       0u,          0u,
                              1024u, 1313429248u, 1313821779u, 2080374784u, 195u, 1224u, 1330400257u, 17497u,      2080505856u, 195u, 1024u, 1162625792u, 1414415693u, 2080636928u, 195u, 1424u, 1162625280u, 67u,         2583953408u, 195u, 1624u, 1447382530u, 1162628169u, 2416312320u, 195u, 0u,    0u,          0u,       0u,          0u,   1024u, 1396787712u, 20047u,      2080374784u, 195u,     1224u, 1229277953u, 82u,         2080505856u, 195u,     1024u, 1380271872u, 1146442324u, 2080636997u, 195u,     1424u, 1279609600u, 5130582u,    2583953408u, 195u,     1624u, 1111966978u, 5526085u,    2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,   1024u, 1163154176u, 1313163344u, 2080374784u, 195u,     1224u, 1447381761u, 20041u,      2080505856u, 195u,     2024u, 1481984000u, 5850444u,    2080636928u, 195u,     1424u, 1380008960u, 5129554u,    2583953408u, 195u,     1064u, 1230128386u, 67u,         2416312320u, 195u,     1024u, 1380008960u, 5129554u, 2080768000u, 195u,
                              1024u, 1128879616u, 1230131028u, 2080374849u, 195u, 1224u, 1296648705u, 22861u,      2080505856u, 195u, 1024u, 1162363648u, 1095062348u, 2080636928u, 195u, 1424u, 1213155840u, 78u,         2583953408u, 195u, 1624u, 1380467458u, 21321u,      2416312320u, 195u, 0u,    0u,          0u,       0u,          0u,   1024u, 1128878592u, 1146241352u, 2080374784u, 195u,     1224u, 1497451009u, 0u,          2080505856u, 195u,     1024u, 1263095040u, 69u,         2080636928u, 195u,     1424u, 1262836224u, 69u,         2583953408u, 195u,     1624u, 1380009730u, 4740165u,    2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,   1024u, 1312901376u, 0u,          2080374784u, 195u,     1224u, 1145656065u, 5850446u,    2080505856u, 195u,     1024u, 1162368000u, 4279628u,    2080636928u, 195u,     1424u, 1095910912u, 19278u,      2583953408u, 195u,     1624u, 1414676738u, 5395784u,    2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,
                              1024u, 1314475008u, 5132100u,    2080374784u, 195u, 1224u, 1279345409u, 5130582u,    2080505856u, 195u, 1024u, 1330599680u, 1464816196u, 2080636928u, 195u, 1424u, 1380272128u, 1414677826u, 2583953408u, 195u, 1624u, 1380009986u, 22866u,      2416312320u, 195u, 0u,    0u,          0u,       0u,          0u,   1024u, 1413565696u, 84u,         2080374784u, 195u,     1224u, 1229734145u, 5132115u,    2080505856u, 195u,     1024u, 1329941504u, 1146241358u, 2080636928u, 195u,     1424u, 1430343680u, 76u,         2583953408u, 195u,     1624u, 1128356354u, 1498562888u, 2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,   1024u, 1313819904u, 22868u,      2080374784u, 195u,     1224u, 1413565185u, 69u,         2080505856u, 195u,     1024u, 1380273664u, 79u,         2080636928u, 195u,     1424u, 1397637376u, 19525u,      2583953408u, 195u,     1624u, 1431258114u, 1396788295u, 2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,
                              1024u, 1430342656u, 4998482u,    2080374784u, 195u, 1224u, 1163281665u, 0u,          2080505856u, 195u, 1024u, 1413825024u, 22868u,      2080636928u, 195u, 1424u, 1313427968u, 20295u,      2583953408u, 195u, 1624u, 1145324802u, 0u,          2416312320u, 195u, 0u,    0u,          0u,       0u,          0u,   1024u, 1229278720u, 76u,         2080374784u, 195u,     1224u, 1515541249u, 1162759745u, 2080505856u, 195u,     1024u, 1380930048u, 89u,         2080636928u, 195u,     1424u, 1230129920u, 1313428563u, 2583953408u, 195u,     1624u, 1330398722u, 1129203026u, 2416312389u, 195u,     0u,    0u,          0u,       0u,          0u,   1024u, 1398096128u, 5130580u,    2080374784u, 195u,     1224u, 1380011265u, 89u,         2080505856u, 195u,     1024u, 1095254784u, 1397050450u, 2080636928u, 195u,     1424u, 1431258112u, 4997447u,    2583953408u, 195u,     1624u, 1162828290u, 0u,          2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,
                              1024u, 1313751296u, 1279607361u, 2080374784u, 195u, 1224u, 1414873601u, 18499u,      2080505856u, 195u, 1024u, 1229014016u, 22867u,      2080636928u, 195u, 1424u, 1314212352u, 69u,         2583953408u, 195u, 1624u, 1314210818u, 5128515u,    2416312320u, 195u, 0u,    0u,          0u,       0u,          0u,   1024u, 1431589632u, 5526081u,    2080374784u, 195u,     1224u, 1313751297u, 69u,         2080505856u, 195u,     1024u, 1280657920u, 16713u,      2080636928u, 195u,     1424u, 1179996672u, 21333u,      2583953408u, 195u,     1624u, 1229475842u, 76u,         2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,   1024u, 1279872000u, 89u,         2080374784u, 195u,     1224u, 1312900097u, 22862u,      2080505856u, 195u,     1024u, 1163219200u, 1313428558u, 2080636928u, 195u,     1424u, 1398096896u, 5130580u,    2583953408u, 195u,     1624u, 1280657922u, 17737u,      2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,
                              1024u, 1230262016u, 83u,         2080374784u, 195u, 1224u, 1430799105u, 19267u,      2080505856u, 195u, 1024u, 1163018496u, 4278356u,    2080636928u, 195u, 1424u, 1464616192u, 4476751u,    2583953408u, 195u, 1624u, 1279874818u, 5132115u,    2416312320u, 195u, 0u,    0u,          0u,       0u,          0u,   1024u, 1095715328u, 1296648259u, 2080374784u, 4194499u, 1224u, 1397705217u, 4739141u,    2080505856u, 2097347u, 1024u, 1263881216u, 69u,         2080636928u, 5701827u, 1424u, 1094992128u, 77u,         2583953408u, 4194499u, 1624u, 1380011266u, 75u,         2416312320u, 1441987u, 0u,    0u,          0u,       0u,          0u,   1024u, 1313429248u, 1313821779u, 2080374784u, 195u,     1224u, 1330400257u, 17497u,      2080505856u, 195u,     1024u, 1162625792u, 1414415693u, 2080636928u, 195u,     1424u, 1162625280u, 67u,         2583953408u, 195u,     1624u, 1447382530u, 1162628169u, 2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,
                              1024u, 1396787712u, 20047u,      2080374784u, 195u, 1224u, 1229277953u, 82u,         2080505856u, 195u, 1024u, 1380271872u, 1146442324u, 2080636997u, 195u, 1424u, 1279609600u, 5130582u,    2583953408u, 195u, 1624u, 1111966978u, 5526085u,    2416312320u, 195u, 0u,    0u,          0u,       0u,          0u,   1024u, 1163154176u, 1313163344u, 2080374784u, 195u,     1224u, 1447381761u, 20041u,      2080505856u, 195u,     2024u, 1481984000u, 5850444u,    2080636928u, 195u,     1424u, 1380008960u, 5129554u,    2583953408u, 195u,     1064u, 1230128386u, 67u,         2416312320u, 195u,     1024u, 1380008960u, 5129554u, 2080768000u, 195u, 1024u, 1128879616u, 1230131028u, 2080374849u, 195u,     1224u, 1296648705u, 22861u,      2080505856u, 195u,     1024u, 1162363648u, 1095062348u, 2080636928u, 195u,     1424u, 1213155840u, 78u,         2583953408u, 195u,     1624u, 1380467458u, 21321u,      2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,
                              1024u, 1128878592u, 1146241352u, 2080374784u, 195u, 1224u, 1497451009u, 0u,          2080505856u, 195u, 1024u, 1263095040u, 69u,         2080636928u, 195u, 1424u, 1262836224u, 69u,         2583953408u, 195u, 1624u, 1380009730u, 4740165u,    2416312320u, 195u, 0u,    0u,          0u,       0u,          0u,   1024u, 1312901376u, 0u,          2080374784u, 195u,     1224u, 1145656065u, 5850446u,    2080505856u, 195u,     1024u, 1162368000u, 4279628u,    2080636928u, 195u,     1424u, 1095910912u, 19278u,      2583953408u, 195u,     1624u, 1414676738u, 5395784u,    2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,   1024u, 1314475008u, 5132100u,    2080374784u, 195u,     1224u, 1279345409u, 5130582u,    2080505856u, 195u,     1024u, 1330599680u, 1464816196u, 2080636928u, 195u,     1424u, 1380272128u, 1414677826u, 2583953408u, 195u,     1624u, 1380009986u, 22866u,      2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,
                              1024u, 1413565696u, 84u,         2080374784u, 195u, 1224u, 1229734145u, 5132115u,    2080505856u, 195u, 1024u, 1329941504u, 1146241358u, 2080636928u, 195u, 1424u, 1430343680u, 76u,         2583953408u, 195u, 1624u, 1128356354u, 1498562888u, 2416312320u, 195u, 0u,    0u,          0u,       0u,          0u,   1024u, 1313819904u, 22868u,      2080374784u, 195u,     1224u, 1413565185u, 69u,         2080505856u, 195u,     1024u, 1380273664u, 79u,         2080636928u, 195u,     1424u, 1397637376u, 19525u,      2583953408u, 195u,     1624u, 1431258114u, 1396788295u, 2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,   1024u, 1430342656u, 4998482u,    2080374784u, 195u,     1224u, 1163281665u, 0u,          2080505856u, 195u,     1024u, 1413825024u, 22868u,      2080636928u, 195u,     1424u, 1313427968u, 20295u,      2583953408u, 195u,     1624u, 1145324802u, 0u,          2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,
                              1024u, 1229278720u, 76u,         2080374784u, 195u, 1224u, 1515541249u, 1162759745u, 2080505856u, 195u, 1024u, 1380930048u, 89u,         2080636928u, 195u, 1424u, 1230129920u, 1313428563u, 2583953408u, 195u, 1624u, 1330398722u, 1129203026u, 2416312389u, 195u, 0u,    0u,          0u,       0u,          0u,   1024u, 1398096128u, 5130580u,    2080374784u, 195u,     1224u, 1380011265u, 89u,         2080505856u, 195u,     1024u, 1095254784u, 1397050450u, 2080636928u, 195u,     1424u, 1431258112u, 4997447u,    2583953408u, 195u,     1624u, 1162828290u, 0u,          2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,   124u,  1313751296u, 1279607361u, 2080374784u, 195u,     124u,  1414873601u, 18499u,      2080505856u, 195u,     124u,  1229014016u, 22867u,      2080636928u, 195u,     124u,  1314212352u, 69u,         2583953408u, 195u,     124u,  1314210818u, 5128515u,    2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,
                              1024u, 1431589632u, 5526081u,    2080374784u, 195u, 1224u, 1313751297u, 69u,         2080505856u, 195u, 1024u, 1280657920u, 16713u,      2080636928u, 195u, 1424u, 1179996672u, 21333u,      2583953408u, 195u, 1624u, 1229475842u, 76u,         2416312320u, 195u, 0u,    0u,          0u,       0u,          0u,   1024u, 1230262016u, 83u,         2080374784u, 195u,     1224u, 1430799105u, 19267u,      2080505856u, 195u,     1024u, 1163018496u, 4278356u,    2080636928u, 195u,     1424u, 1464616192u, 4476751u,    2583953408u, 195u,     1624u, 1279874818u, 5132115u,    2416312320u, 195u,     0u,    0u,          0u,       0u,          0u,
                          }};

static uint32 sort_times(uint32 idx)
{
    uint32 changed = 1;
    uint32 variant;
    RECORD_ENTRY *entry;
    uint32 cur;
    uint32 next;
    uint32 denominator;
    uint32 saved[5];
    uint32 moved[4];
    uint32 word;

    FUNCTION_MARKER(0x80017940, "SLUS_006.10");
    while (changed != 0)
    {
        changed = 0;
        for (variant = 0; variant < 5; ++variant)
        {
            entry = &rec_table.rows[idx][variant];
            cur = entry->data.val * 100;
            denominator = ((sint32)disp_state.field_rate / 2);
            if (denominator == 0)
                xport_mips_break(7);
            cur /= denominator;
            next = entry[1].data.val * 100;
            denominator = ((sint32)disp_state.field_rate / 2);
            if (denominator == 0)
                xport_mips_break(7);
            next /= denominator;
            if ((sint32)next < (sint32)cur)
            {
                ++changed;
                for (word = 0; word < 5; ++word)
                    saved[word] = entry->words[word];
                for (word = 0; word < 4; ++word)
                    moved[word] = entry[1].words[word];
                for (word = 0; word < 4; ++word)
                    entry->words[word] = (uint32)(moved[word]);
                entry->words[4] = (uint32)(entry[1].words[4]);
                for (word = 0; word < 4; ++word)
                    entry[1].words[word] = (uint32)(saved[word]);
                entry[1].words[4] = (uint32)(saved[4]);
            }
        }
    }
    return 0;
}

static uint32 sort_points(uint32 idx)
{
    uint32 changed = 1;
    uint32 variant;
    RECORD_ENTRY *entry;
    uint32 cur;
    uint32 next;
    uint32 saved[5];
    uint32 moved[4];
    uint32 word;

    FUNCTION_MARKER(0x80017CC0, "SLUS_006.10");
    while (changed != 0)
    {
        changed = 0;
        for (variant = 0; variant < 5; ++variant)
        {
            entry = &rec_table.rows[idx][variant];
            cur = entry->data.val;
            next = entry[1].data.val;
            if (cur < next)
            {
                ++changed;
                for (word = 0; word < 5; ++word)
                    saved[word] = entry->words[word];
                for (word = 0; word < 4; ++word)
                    moved[word] = entry[1].words[word];
                for (word = 0; word < 4; ++word)
                    entry->words[word] = (uint32)(moved[word]);
                entry->words[4] = (uint32)(entry[1].words[4]);
                for (word = 0; word < 4; ++word)
                    entry[1].words[word] = (uint32)(saved[word]);
                entry[1].words[4] = (uint32)(saved[4]);
            }
        }
    }
    return 0;
}

static uint32 sort_detail(uint32 idx)
{
    uint32 changed = 1;
    uint32 variant;
    RECORD_ENTRY *entry;
    uint32 cur;
    uint32 next;
    uint32 saved[5];
    uint32 moved[4];
    uint32 word;

    FUNCTION_MARKER(0x80017F90, "SLUS_006.10");
    while (changed != 0)
    {
        changed = 0;
        for (variant = 0; variant < 5; ++variant)
        {
            entry = &rec_table.rows[idx][variant];
            cur = entry->data.detail;
            next = entry[1].data.detail;
            if (cur < next)
            {
                ++changed;
                for (word = 0; word < 5; ++word)
                    saved[word] = entry->words[word];
                for (word = 0; word < 4; ++word)
                    moved[word] = entry[1].words[word];
                for (word = 0; word < 4; ++word)
                    entry->words[word] = (uint32)(moved[word]);
                entry->words[4] = (uint32)(entry[1].words[4]);
                for (word = 0; word < 4; ++word)
                    entry[1].words[word] = (uint32)(saved[word]);
                entry[1].words[4] = (uint32)(saved[4]);
            }
        }
    }
    return 0;
}

uint32 recs_reset_candidates(void)
{
    uint32 idx;
    uint32 offset;

    FUNCTION_MARKER(0x80016344, "SLUS_006.10");
    for (idx = 0; idx < 47; ++idx)
    {
        offset = idx * 120;
        rec_table.rows[idx][5].data.val = (uint32)(idx == 44 ? 0 : UINT32_MAX);
        rec_table.rows[idx][5].data.name_state = (uint8)(0);
        rec_table.rows[idx][5].data.name[0] = (uint8)(0);
        rec_table.rows[idx][5].data.model = (uint8)(0);
        rec_table.rows[idx][5].data.speed = (uint16)(0);
        rec_table.rows[idx][5].data.peak_speed = (uint16)(0);
        rec_table.rows[idx][5].data.detail = (uint8)(0);
    }
    return 47;
}

uint32 recs_generate_defaults(void)
{
    uint32 base_speeds[47];
    uint32 pitch_table[16];
    uint32 weights[47];
    uint32 idx;
    uint32 variant;
    uint32 offset;
    uint32 val;
    uint32 pitch;
    uint32 denominator;
    uint32 ratio;

    FUNCTION_MARKER(0x80016FC4, "SLUS_006.10");
    for (idx = 0; idx < 47; ++idx)
        base_speeds[idx] = initial_vals[idx];
    for (idx = 0; idx < 16; ++idx)
        pitch_table[idx] = model_speeds[idx];
    for (idx = 0; idx < 47; ++idx)
        weights[idx] = rec_weights[idx];
    car_random_states[0] = (uint32)(0x3AFE5F64);
    for (idx = 0; idx < 47; ++idx)
    {
        for (variant = 0; variant < 5; ++variant)
        {
            offset = idx * 120 + variant * 20;
            val = car_random_next(0);
            rec_table.rows[idx][variant].data.val = (uint32)(base_speeds[idx] + val % 300);
            val = car_random_next(0);
            rec_table.rows[idx][variant].data.model = (uint8)((uint8)(val % 14));
            val = car_random_next(0);
            pitch = pitch_table[rec_table.rows[idx][variant].data.model] & 0xFFFF;
            rec_table.rows[idx][variant].data.peak_speed = (uint16)((uint16)(pitch - val % 48));
            ratio = ((sint32)(weights[idx] * 61440) / 5280);
            denominator = ((rec_table.rows[idx][variant].data.val << 12) >> 5) / 3375;
            if (denominator == 0)
                xport_mips_break(7);
            ratio /= denominator;
            pitch = pitch_table[rec_table.rows[idx][variant].data.model] & 0xFFFF;
            rec_table.rows[idx][variant].data.speed = (uint16)((uint16)(((ratio * 1252) >> 8) - 904 + pitch));
            if (idx == 29)
            {
                rec_table.rows[idx][variant].data.speed = (uint16)((uint16)((rec_table.rows[idx][variant].data.peak_speed >> 2) * 2));
                rec_table.rows[idx][variant].data.peak_speed = (uint16)((uint16)(rec_table.rows[idx][variant].data.peak_speed - 244));
                val = car_random_next(0);
                rec_table.rows[idx][variant].data.val = (uint32)((val % 5000 + base_speeds[idx]) * 24);
            }
            else if (idx == 40)
            {
                val = car_random_next(0);
                rec_table.rows[idx][variant].data.detail = (uint8)((uint8)((val & 3) + variant * 5 + 74));
            }
            else if (idx == 44)
            {
                val = car_random_next(0);
                rec_table.rows[idx][variant].data.val = (uint32)(val % 1000 + variant * 1200 + 58000);
            }
            else if (idx >= 41)
            {
                val = car_random_next(0);
                rec_table.rows[idx][variant].data.val = (uint32)(base_speeds[idx] + val % 6000);
                val = car_random_next(0);
                rec_table.rows[idx][variant].data.detail = (uint8)((uint8)(val % 33 + 10));
            }
        }
    }
    return 47;
}

uint32 recs_sort(void)
{
    uint32 idx;

    FUNCTION_MARKER(0x80016EF8, "SLUS_006.10");
    for (idx = 0; idx < 47; ++idx)
    {
        if (idx == 44)
            sort_points(idx);
        else if (idx == 40)
            sort_detail(idx);
        else
            sort_times(idx);
    }
    return 47;
}

uint32 recs_update_candidate(sint32 rec)
{
    uint32 offset = (uint32)rec * 120;
    uint32 val, prev, idx;

    FUNCTION_MARKER(0x800164E0, "SLUS_006.10");
    if (rec < 40)
    {
        val = car_states[0].finish_time;
        prev = rec_table.rows[(uint32)rec][5].data.val;
        if (val < prev)
        {
            val = car_states[0].finish_time;
            rec_table.rows[(uint32)rec][5].data.val = (uint32)(val);
            rec_table.rows[(uint32)rec][5].data.name_state = (uint8)(0);
            rec_table.rows[(uint32)rec][5].data.name[0] = (uint8)(0);
            val = session_car_choices.cars[0].model[0];
            rec_table.rows[(uint32)rec][5].data.model = (uint8)(val);
            val = car_states[0].race_stats.avg_speed;
            rec_table.rows[(uint32)rec][5].data.speed = (uint16)(val);
            val = car_states[0].race_stats.top_speed;
            rec_table.rows[(uint32)rec][5].data.peak_speed = (uint16)(val);
            rec_table.rows[(uint32)rec][5].data.detail = (uint8)(0);
        }
        val = race_session.linear_course;
        if (val == 0)
            return val;
        val = race_session.drag_race;
        if (val != 0)
            return val;
        val = car_states[0].split_ticks[0];
        rec_table.rows[(uint32)rec][5].data.val = (uint32)(val);
        for (idx = 1; idx < 5; ++idx)
        {
            val = car_states[0].split_ticks[idx];
            if (val != 0)
            {
                val = car_states[0].split_ticks[idx];
                prev = rec_table.rows[(uint32)rec][5].data.val;
                if (val < prev)
                {
                    val = car_states[0].split_ticks[idx];
                    rec_table.rows[(uint32)rec][5].data.val = (uint32)(val);
                }
            }
        }
        return 5;
    }
    val = (rec == 44 ? race_results.entries[0].points.val : race_results.entries[0].total_time);
    rec_table.rows[(uint32)rec][5].data.val = (uint32)(val);
    rec_table.rows[(uint32)rec][5].data.name_state = (uint8)(0);
    rec_table.rows[(uint32)rec][5].data.name[0] = (uint8)(0);
    val = race_results.entries[0].model;
    rec_table.rows[(uint32)rec][5].data.model = (uint8)(val);
    val = race_results.entries[0].speed_sum;
    rec_table.rows[(uint32)rec][5].data.speed = (uint16)(val);
    val = race_results.entries[0].top_speed;
    rec_table.rows[(uint32)rec][5].data.peak_speed = (uint16)(val);
    val = race_results.entries[0].points.bytes[0];
    rec_table.rows[(uint32)rec][5].data.detail = (uint8)(val);
    return offset;
}

uint64 recs_set_candidate_names(void)
{
    uint32 idx;
    const char *src;

    FUNCTION_MARKER(0x80016A28, "SLUS_006.10");
    for (idx = 0; idx < 47; ++idx)
    {
        src = presentation_player_name;
        memcpy(rec_table.rows[idx][5].data.name, src, 8);
        rec_table.rows[idx][5].data.name_state = (uint8)(1);
    }
    return 47;
}

uint64 recs_clear_name_states(void)
{
    uint32 idx, offset;

    FUNCTION_MARKER(0x80016AF4, "SLUS_006.10");
    for (idx = 0; idx < 47; ++idx)
    {
        offset = idx * 120;
        rec_table.rows[idx][0].data.name_state = (uint8)(0);
        rec_table.rows[idx][1].data.name_state = (uint8)(0);
        rec_table.rows[idx][2].data.name_state = (uint8)(0);
        rec_table.rows[idx][3].data.name_state = (uint8)(0);
        rec_table.rows[idx][4].data.name_state = (uint8)(0);
        rec_table.rows[idx][5].data.name_state = (uint8)(0);
    }
    return 47;
}

uint32 recs_has_candidate(void)
{
    uint32 idx, offset, cur, prev;

    FUNCTION_MARKER(0x80016C2C, "SLUS_006.10");
    if (race_session.event_mode == 0 && race_session.linear_course == 0 && (race_rules.checkpoint_timers == 0 || race_rules.traffic == 0 || race_rules.cops == 0))
        return 0;
    for (idx = 0; idx < 47; ++idx)
    {
        offset = idx * 120;
        if (idx == 40)
        {
            cur = rec_table.rows[idx][5].data.detail;
            prev = rec_table.rows[idx][4].data.detail;
        }
        else
        {
            cur = rec_table.rows[idx][5].data.val;
            prev = rec_table.rows[idx][4].data.val;
        }
        if (idx == 40 || idx == 44)
        {
            if (cur >= prev && cur != 0)
                return 1;
        }
        else if (prev >= cur && cur != 0)
            return 1;
    }
    return 0;
}
