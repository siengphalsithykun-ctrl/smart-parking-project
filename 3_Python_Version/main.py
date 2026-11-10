"""
SMART PARKING MANAGEMENT SYSTEM  (Python version)
COSC 111 - Final Python Project

Problem:  A parking attendant tracks free spaces, where each vehicle is,
          and how much each driver owes on paper. That is slow and
          error-prone (e.g. the same plate written down twice).
Solution: A menu program for a 10-space lot that parks and removes
          vehicles, calculates fees, prints receipts, searches plates,
          and keeps a history of today's income.

Fees: bike $0.50 per hour, car $1.50 per hour, minimum charge 1 hour.
Uses only built-in Python - no packages to install.
"""

# ---------------------------------------------------------------
# Settings and data
# ---------------------------------------------------------------
MAX_SPACES = 10        # number of spaces in the lot
START_HOUR = 8         # the clock starts at 8:00

# Dictionary: vehicle type -> hourly rate. Adding a new type only
# needs one new line here.
RATES = {
    "bike": 0.50,
    "car": 1.50,
}

# List of spaces. Each space is None (free) or a dictionary that
# describes the parked vehicle: {"plate", "type", "entry"}.
# Index i in the list = space number i + 1.
spaces = [None] * MAX_SPACES

# List of receipts for vehicles that have already left (history).
history = []

# The simple clock, stored in a dictionary so functions can change it.
clock = {"hour": START_HOUR}


# ---------------------------------------------------------------
# Search helpers
# ---------------------------------------------------------------
def find_free_space():
    """Return the index of the first free space, or -1 if the lot is full."""
    for i in range(MAX_SPACES):
        if spaces[i] is None:
            return i
    return -1


def find_by_plate(plate):
    """Linear search: return the index of the space holding this plate, or -1."""
    for i in range(MAX_SPACES):
        if spaces[i] is not None and spaces[i]["plate"] == plate:
            return i
    return -1


def calculate_fee(vehicle_type, entry_hour, exit_hour):
    """Return (hours, rate, fee). Anything under 1 hour is charged as 1 hour."""
    hours = exit_hour - entry_hour
    if hours < 1:
        hours = 1
    rate = RATES[vehicle_type]
    fee = hours * rate
    return hours, rate, fee


# ---------------------------------------------------------------
# Input helpers (validation)
# ---------------------------------------------------------------
def ask_plate():
    """Ask for a plate until the user types something that is not empty."""
    while True:
        plate = input("Plate: ").strip().upper()   # "ab123" and "AB123" are the same car
        if plate == "":
            print("Plate cannot be empty.")
        elif " " in plate:
            print("Plate cannot contain spaces.")
        else:
            return plate


def ask_positive_int(prompt):
    """Ask for a whole number of 1 or more. Returns None if the input is invalid."""
    text = input(prompt).strip()
    if not text.isdigit():               # rejects letters, "-3", "2.5" and blanks
        print("Please enter a whole number of 1 or more.")
        return None
    number = int(text)
    if number < 1:
        print("Please enter a whole number of 1 or more.")
        return None
    return number


# ---------------------------------------------------------------
# Main features
# ---------------------------------------------------------------
def park_vehicle():
    """Feature 1: park a vehicle in the first free space."""
    space = find_free_space()
    if space == -1:
        print("Sorry, the lot is full.")
        return

    plate = ask_plate()
    if find_by_plate(plate) != -1:
        print(plate, "is already parked.")
        return

    vehicle_type = input("Type (bike/car): ").strip().lower()
    if vehicle_type not in RATES:
        print("Invalid type. Use bike or car.")
        return

    spaces[space] = {"plate": plate, "type": vehicle_type, "entry": clock["hour"]}
    print(f"{plate} parked in space {space + 1} at {clock['hour']}:00.")


def remove_vehicle():
    """Feature 2: remove a vehicle, print its receipt and save it to history."""
    plate = ask_plate()
    space = find_by_plate(plate)
    if space == -1:
        print("Vehicle", plate, "not found.")
        return

    vehicle = spaces[space]
    hours, rate, fee = calculate_fee(vehicle["type"], vehicle["entry"], clock["hour"])

    print("\n======= RECEIPT =======")
    print(f"Plate:  {vehicle['plate']}")
    print(f"Type:   {vehicle['type']}")
    print(f"Space:  {space + 1}")
    print(f"In:     {vehicle['entry']}:00")
    print(f"Out:    {clock['hour']}:00")
    print(f"Hours:  {hours}")
    print(f"Rate:   ${rate:.2f} per hour")
    print(f"Fee:    ${fee:.2f}")
    print("=======================")

    # Keep a copy of the receipt, then free the space.
    history.append({
        "plate": vehicle["plate"],
        "type": vehicle["type"],
        "hours": hours,
        "fee": fee,
    })
    spaces[space] = None


def show_lot():
    """Feature 3: print every space, then how many are busy and free."""
    busy = 0
    print(f"\n--- Parking Lot (clock {clock['hour']}:00) ---")
    print("Space\tStatus\tPlate\tType\tEntry")
    for i in range(MAX_SPACES):
        vehicle = spaces[i]
        if vehicle is not None:
            print(f"{i + 1}\tBUSY\t{vehicle['plate']}\t{vehicle['type']}\t{vehicle['entry']}:00")
            busy += 1
        else:
            print(f"{i + 1}\tFREE\t-\t-\t-")
    print(f"Busy: {busy}\tFree: {MAX_SPACES - busy}")


def search_vehicle():
    """Feature 4: find which space a plate is in, and what it owes so far."""
    plate = ask_plate()
    space = find_by_plate(plate)
    if space == -1:
        print(plate, "not found.")
        return

    vehicle = spaces[space]
    hours, rate, fee = calculate_fee(vehicle["type"], vehicle["entry"], clock["hour"])
    print(f"{plate} ({vehicle['type']}) is in space {space + 1}, "
          f"parked since {vehicle['entry']}:00. Due now: ${fee:.2f}")


def advance_clock():
    """Feature 5 (demo): move the clock forward to simulate time passing."""
    hours = ask_positive_int("Hours to advance: ")
    if hours is None:
        return
    clock["hour"] = clock["hour"] + hours
    print(f"Clock is now {clock['hour']}:00.")


def show_history():
    """Feature 6: list every vehicle that has left and the total income."""
    if len(history) == 0:
        print("No vehicles have left yet.")
        return

    total = 0
    print("\n--- Today's Records ---")
    print("No.\tPlate\tType\tHours\tFee")
    for number, record in enumerate(history, start=1):
        print(f"{number}\t{record['plate']}\t{record['type']}\t{record['hours']}\t${record['fee']:.2f}")
        total += record["fee"]
    print(f"Vehicles: {len(history)}\tTotal income: ${total:.2f}")


# ---------------------------------------------------------------
# Menu
# ---------------------------------------------------------------
def main():
    """Show the menu and run the chosen feature until the user exits."""
    # Dictionary: menu choice -> the function that handles it.
    actions = {
        "1": park_vehicle,
        "2": remove_vehicle,
        "3": show_lot,
        "4": search_vehicle,
        "5": advance_clock,
        "6": show_history,
    }

    while True:
        print(f"\n===== SMART PARKING (clock {clock['hour']}:00) =====")
        print("1. Park vehicle")
        print("2. Remove vehicle (receipt)")
        print("3. Show lot")
        print("4. Search plate")
        print("5. Advance clock")
        print("6. History & income")
        print("0. Exit")
        choice = input("Choice: ").strip()

        if choice == "0":
            print("Goodbye!")
            break
        elif choice in actions:
            actions[choice]()          # call the matching feature
        else:
            print("Invalid choice. Please enter a number from 0 to 6.")


# Run the program only when this file is started directly.
if __name__ == "__main__":
    main()
