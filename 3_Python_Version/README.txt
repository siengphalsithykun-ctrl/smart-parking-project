SMART PARKING MANAGEMENT SYSTEM - COSC 111 Final Python Project
================================================================

Group: Group__            Video demo link: ____________________

Members (name - ID):
  1. ____________________   2. ____________________
  3. ____________________   4. ____________________
  5. ____________________

HOW TO RUN
----------
Needs Python 3 (any recent version). No extra packages are required.
Open a terminal in this folder and type:
    python main.py
Then choose an option from the menu. Data is kept in memory only, so
closing the program empties the lot.

FEATURES
--------
1. Park vehicle      - first free space; blocks duplicate plates and bad types
2. Remove vehicle    - calculates the fee (min. 1 hour) and prints a receipt
3. Show lot          - every space, busy/free count
4. Search plate      - space, entry time and amount due now
5. Advance clock     - simulates time passing for the demo
6. History & income  - list of vehicles that left and total income
Fees: bike $0.50/hour, car $1.50/hour.

Example workflow: 1 (park AB123, car) -> 5 (advance 3 hours) ->
4 (search AB123) -> 2 (remove AB123, receipt $4.50) -> 6 (history).

PYTHON CONCEPTS USED
--------------------
- Lists: `spaces` (one entry per parking space) and `history` (receipts)
- Dictionaries: each parked vehicle, the RATES table, and the menu
  `actions` table that maps a choice to its function
- Conditions: input checks, full lot, duplicate plate, 1-hour minimum
- Loops: for loops for linear search and display, while loops for the
  menu and for asking again when a plate is empty
- Functions: 12 user-defined functions, one job each

MEMBER CONTRIBUTIONS
--------------------
Member 1 - data design (spaces list, vehicle dictionaries),
           find_free_space(), find_by_plate(), show_lot()
Member 2 - park_vehicle() and its checks, RATES dictionary
Member 3 - calculate_fee(), remove_vehicle() and the receipt
Member 4 - search_vehicle(), advance_clock(), show_history()
Member 5 - main() menu, input validation (ask_plate,
           ask_positive_int), testing, slides and video
