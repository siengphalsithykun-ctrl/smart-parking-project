#include <iostream>
#include <string>
using namespace std;

const int MAX_SPACES = 10; // number of spaces in the lot

class ParkingLot
{
private:
    bool occupied[MAX_SPACES]; // true = a vehicle is parked here
    string plates[MAX_SPACES]; // licence plate of the vehicle
    string types[MAX_SPACES];  // "bike" or "car"
    int entryHour[MAX_SPACES]; // hour the vehicle came in
    int clockHour;             // our simple "clock" (whole hours)

public:
    // Constructor: start with an empty lot and the clock at 8:00.
    ParkingLot()
    {
        clockHour = 8;
        for (int i = 0; i < MAX_SPACES; i++)
        {
            occupied[i] = false;
            plates[i] = "";
            types[i] = "";
            entryHour[i] = 0;
        }
    }

    // Return the index of the first free space, or -1 if the lot is full.
    int findFree()
    {
        for (int i = 0; i < MAX_SPACES; i++)
        {
            if (!occupied[i])
            {
                return i;
            }
        }
        return -1;
    }

    int findByPlate(string plate)
    {
        for (int i = 0; i < MAX_SPACES; i++)
        {
            if (occupied[i] && plates[i] == plate)
            {
                return i;
            }
        }
        return -1;
    }

    // Print every space, then how many are busy and free.
    void showLot()
    {
        int busy = 0;
        cout << "\n--- Parking Lot (clock " << clockHour << ":00) ---\n";
        cout << "Space\tStatus\tPlate\tType\tEntry\n";
        for (int i = 0; i < MAX_SPACES; i++)
        {
            if (occupied[i])
            {
                cout << i + 1 << "\tBUSY\t" << plates[i] << "\t"
                     << types[i] << "\t" << entryHour[i] << ":00\n";
                busy++;
            }
            else
            {
                cout << i + 1 << "\tFREE\t-\t-\t-\n";
            }
        }
        cout << "Busy: " << busy << "\tFree: " << MAX_SPACES - busy << "\n";
    }

    bool isValidType(string type)
    {
        return type == "bike" || type == "car";
    }

    // Hourly rate for each vehicle type.
    double getRate(string type)
    {
        if (type == "bike")
        {
            return 0.50;
        }
        else
        {
            return 1.50; // car
        }
    }

    void park(string plate, string type)
    {
        int space = findFree();
        if (space == -1)
        {
            cout << "Sorry, the lot is full.\n";
            return;
        }
        if (!isValidType(type))
        {
            cout << "Invalid type. Use bike or car.\n";
            return;
        }
        if (findByPlate(plate) != -1)
        {
            cout << "Already parked.\n";
            return;
        }

        occupied[space] = true;
        plates[space] = plate;
        types[space] = type;
        entryHour[space] = clockHour;
        cout << plate << " parked in space " << space + 1
             << " at " << clockHour << ":00.\n";
    }

    void remove(string plate)
    {
        int space = findByPlate(plate);
        if (space == -1)
        {
            cout << "Vehicle " << plate << " not found.\n";
            return;
        }

        int hours = clockHour - entryHour[space];
        if (hours < 1)
        {
            hours = 1;
        }
        double rate = getRate(types[space]);
        double fee = hours * rate;

        cout << "\n======= RECEIPT =======\n";
        cout << "Plate:\t" << plates[space] << "\n";
        cout << "Type:\t" << types[space] << "\n";
        cout << "Space:\t" << space + 1 << "\n";
        cout << "In:\t" << entryHour[space] << ":00\n";
        cout << "Out:\t" << clockHour << ":00\n";
        cout << "Hours:\t" << hours << "\n";
        cout << "Rate:\t$" << rate << " per hour\n";
        cout << "Fee:\t$" << fee << "\n";
        cout << "=======================\n";

        occupied[space] = false;
        plates[space] = "";
        types[space] = "";
        entryHour[space] = 0;
    }

    // Demo feature: move the clock forward by n hours.
    void advance(int n)
    {
        if (n < 1)
        {
            cout << "Please enter a positive number of hours.\n";
            return;
        }
        clockHour = clockHour + n;
        cout << "Clock is now " << clockHour << ":00.\n";
    }

    void search(string plate)
    {
        int space = findByPlate(plate);
        if (space == -1)
        {
            cout << plate << " not found.\n";
        }
        else
        {
            cout << plate << " (" << types[space] << ") is in space "
                 << space + 1 << ", parked since " << entryHour[space] << ":00.\n";
        }
    }

    int getClock()
    {
        return clockHour;
    }
};

int main()
{
    ParkingLot lot;

    int choice = -1;
    while (choice != 0)
    {
        cout << "\n===== SMART PARKING (clock " << lot.getClock() << ":00) =====\n";
        cout << "1. Park\n2. Remove\n3. Show\n4. Search\n5. Advance\n0. Exit\n";
        cout << "Choice: ";
        cin >> choice;

        if (cin.fail())
        {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "Please enter a number from the menu.\n";
            choice = -1;
            continue;
        }

        string plate, type;
        if (choice == 1)
        {
            cout << "Plate: ";
            cin >> plate;
            cout << "Type (bike/car): ";
            cin >> type;
            lot.park(plate, type);
        }
        else if (choice == 2)
        {
            cout << "Plate: ";
            cin >> plate;
            lot.remove(plate);
        }
        else if (choice == 3)
        {
            lot.showLot();
        }
        else if (choice == 4)
        {
            cout << "Plate: ";
            cin >> plate;
            lot.search(plate);
        }
        else if (choice == 5)
        {
            int n;
            cout << "Hours to advance: ";
            cin >> n;
            if (cin.fail())
            {
                cin.clear();
                cin.ignore(1000, '\n');
                cout << "Please enter a number.\n";
            }
            else
            {
                lot.advance(n);
            }
        }
        else if (choice == 0)
        {
            cout << "Goodbye!\n";
        }
        else
        {
            cout << "Invalid choice.\n";
        }
    }
    return 0;
}
