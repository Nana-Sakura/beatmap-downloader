#include <cli.h>
#include <stdio.h>
#include <assert.h>

const char* modes[] = { "osu", "taiko", "catch", "mania" };

get_t
userInput(void)
{
  get_t res;
  int r;
  printf("Input the uid you want to get beatmap from: ");
  r = scanf("%d", &res.uid);
  assert(r == 1);
  puts("Choose mode: (input 0-3)");
  puts("0: osu; 1: taiko; 2: catch; 3: mania;");
  printf("Your choice: ");
  r = scanf("%d", &res.mode);
  assert(r == 1 && (res.mode >= 0 && res.mode <= 3));
  res._mode = modes[res.mode];
  puts("Consider Lazer? (0: Yes; 1: No;)");
  printf("Your choice: ");
  r = scanf("%d", &res.legacy_only);
  assert(r == 1 && (res.legacy_only >= 0 && res.legacy_only <= 1));
  return res;
}