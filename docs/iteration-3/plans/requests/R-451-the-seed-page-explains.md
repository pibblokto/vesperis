# R-451: the seed page explains

status: done 2026-11-22 (1.5.0, before its release)
milestone: 1.5.0

## Request

The user, 2026-11-22: "Why organism has nothing to eat when drone tries to seed it", and on the plan, which offered the seed page's
needs and a failed seeding's reason in full: "Proceed. ... The idea is to make it more user friendly, more transparent, easy to
understand and play with".

## Done

- **What the culture needs.** Under CULTURE the seed page has NEEDS: its liquid, its food, its salt, its warmths and whether it lives
  dry (`WATER, DEAD MATTER, FRESH, 7 TO 47 C, DRY OK`).
- **What the place gives.** Under PLACE it has THERE: the place's liquid, its food, light or dark, its salt, its reading and whether
  it is dry land (`WATER, DEAD MATTER, LIT, FRESH, 50 C, DRY`). The page gives no forecast: the drone finds out.
- **Why it died, in full.** The drone's report says the verdict, then the reason with the culture's need against the place's
  (`TOO HOT: IT GROWS UP TO 47 C, THE PLACE IS 50 C`, `NO FOOD: IT EATS DEAD MATTER, THE PLACE GIVES HYDROGEN`, `WRONG LIQUID: IT LIVES
  IN WATER, THE PLACE HOLDS ACID`, `RADIATION: IT TAKES 1.2 GY A YEAR, THE PLACE GETS 300`), in the log too and on the drone bay's list.
  A save keeps it (`bayseedwhy`).
- **One path:** the place's facts (its medium, oxygen, pH and dose) are the verdict's first part (`seedPlaceFacts`), so the page and the
  verdict read the same place.
- **Verified:** see the log entry R-451.
