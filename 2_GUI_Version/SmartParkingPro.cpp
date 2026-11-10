// =============================================================
//  Smart Parking PRO - Windows GUI edition
//
//  New in this version:
//   * Choose your own space: click a free space, then press Park.
//   * Three zones, each on its own tab: Standard (x1.0),
//     Premium (x1.5) and VIP (x2.0), with 10 spaces each.
//   * Membership plans: Guest, Silver, Gold, Platinum.
//   * Add, change and remove memberships.
//
//  Main concepts used: classes, arrays (lots of parallel arrays),
//  loops, conditions and functions. The window uses the Win32 API
//  that comes with Windows, so nothing extra has to be installed.
//
//  Build with MinGW:
//    g++ SmartParkingPro.cpp -o SmartParkingPro.exe -mwindows -lgdi32
// =============================================================

#define _WIN32_WINNT 0x0501
#define WINVER 0x0501
#include <windows.h>
#include <string>
using namespace std;

const int SPACES_PER_TIER = 10;   // spaces on each zone tab
const int MAX_SPACES  = 30;       // 3 zones x 10 spaces
const int MAX_MEMBERS = 20;

// =============================================================
//  PART 1 - Prices, zones and plans (all stored in arrays)
// =============================================================

// Base price per hour for each vehicle type
const double CAR_RATE  = 1.50;
const double BIKE_RATE = 0.50;

// Zones ("tiers") of parking spaces: 0 = Standard, 1 = Premium, 2 = VIP
const int TIER_COUNT = 3;
const string tierName[TIER_COUNT]       = {"STANDARD", "PREMIUM", "VIP"};
const string tierTitle[TIER_COUNT]      = {"Standard", "Premium", "VIP"};
const double tierMultiplier[TIER_COUNT] = {1.0, 1.5, 2.0};

// Membership plans: 0 = Guest, 1 = Silver, 2 = Gold, 3 = Platinum
const int PLAN_COUNT = 4;
const string planName[PLAN_COUNT]     = {"Guest", "Silver", "Gold", "Platinum"};
const double planPrice[PLAN_COUNT]    = {0, 5, 12, 25};          // per month
const double planDiscount[PLAN_COUNT] = {0, 0.10, 0.20, 0.30};   // off parking
const bool   planVip[PLAN_COUNT]      = {false, false, true, true};
const bool   planFreeHour[PLAN_COUNT] = {false, false, false, true};

// =============================================================
//  PART 2 - Helper functions (numbers into text)
// =============================================================

// 42 -> "42" (take the last digit with % 10, remove it with / 10)
string intToText(int n) {
    if (n == 0) {
        return "0";
    }
    string text = "";
    while (n > 0) {
        char digit = '0' + (n % 10);
        text = digit + text;
        n = n / 10;
    }
    return text;
}

// 4.5 -> "$4.50"
string moneyText(double amount) {
    int cents = (int)(amount * 100 + 0.5);
    string centText = intToText(cents % 100);
    if (cents % 100 < 10) {
        centText = "0" + centText;
    }
    return "$" + intToText(cents / 100) + "." + centText;
}

// 8 -> "8:00"
string hourText(int hour) {
    return intToText(hour) + ":00";
}

// 1.5 -> "x1.5"
string multiplierText(double m) {
    int tenths = (int)(m * 10 + 0.5);
    return "x" + intToText(tenths / 10) + "." + intToText(tenths % 10);
}

// 0.2 -> "20%"
string percentText(double p) {
    return intToText((int)(p * 100 + 0.5)) + "%";
}

// Space index -> its name, for example 13 -> "Premium 4".
// Spaces 0-9 are Standard, 10-19 Premium, 20-29 VIP.
string spaceLabel(int i) {
    return tierTitle[i / SPACES_PER_TIER] + " " + intToText(i % SPACES_PER_TIER + 1);
}

double baseRate(string type) {
    if (type == "bike") {
        return BIKE_RATE;
    }
    return CAR_RATE;
}

// =============================================================
//  PART 3 - MemberList class: who has which membership plan
// =============================================================
class MemberList {
private:
    string plates[MAX_MEMBERS];   // parallel arrays again:
    int    plans[MAX_MEMBERS];    // plates[k] has plan plans[k]
    int    count;
    double sales;                 // money from membership fees

public:
    MemberList() {
        count = 0;
        sales = 0;
    }

    // Position of a plate in the list, or -1.
    int find(string plate) {
        for (int k = 0; k < count; k++) {
            if (plates[k] == plate) {
                return k;
            }
        }
        return -1;
    }

    // Plan of a plate (0 = Guest if it is not a member).
    int planOf(string plate) {
        int k = find(plate);
        if (k == -1) {
            return 0;
        }
        return plans[k];
    }

    // Remove a membership: delete it from the arrays by moving
    // every member after it one step to the left.
    string removeMember(string plate) {
        int k = find(plate);
        if (k == -1) {
            return plate + " is not a member.";
        }
        string oldPlan = planName[plans[k]];
        for (int j = k; j < count - 1; j++) {
            plates[j] = plates[j + 1];
            plans[j] = plans[j + 1];
        }
        count--;
        return plate + "'s " + oldPlan + " membership was removed.";
    }

    // Join, change or cancel (plan 0) a membership.
    string join(string plate, int plan) {
        int k = find(plate);

        if (plan == 0) {
            return removeMember(plate);
        }

        if (k != -1 && plans[k] == plan) {
            return plate + " is already a " + planName[plan] + " member.";
        }
        if (k == -1 && count == MAX_MEMBERS) {
            return "The member list is full.";
        }

        sales = sales + planPrice[plan];
        if (k == -1) {
            plates[count] = plate;
            plans[count] = plan;
            count++;
            return plate + " joined " + planName[plan] + " for "
                   + moneyText(planPrice[plan]) + " / month.";
        }
        plans[k] = plan;
        return plate + " changed to " + planName[plan] + " ("
               + moneyText(planPrice[plan]) + " / month).";
    }

    int    getCount()      { return count; }
    string getPlate(int k) { return plates[k]; }
    int    getPlan(int k)  { return plans[k]; }
    double getSales()      { return sales; }
};

// =============================================================
//  PART 4 - ParkingLot class: the spaces and the parking fees
// =============================================================
class ParkingLot {
private:
    bool   occupied[MAX_SPACES];
    string plates[MAX_SPACES];
    string types[MAX_SPACES];     // "bike" or "car"
    int    entryHour[MAX_SPACES];
    int    tiers[MAX_SPACES];     // zone of each space
    int    clockHour;
    double totalCollected;

public:
    // Every space belongs to a zone: 10 Standard, 10 Premium, 10 VIP.
    ParkingLot() {
        clockHour = 8;
        totalCollected = 0;
        for (int i = 0; i < MAX_SPACES; i++) {
            occupied[i] = false;
            plates[i] = "";
            types[i] = "";
            entryHour[i] = 0;
            tiers[i] = i / SPACES_PER_TIER;   // 0-9 Standard, 10-19 Premium, 20-29 VIP
        }
    }

    int findByPlate(string plate) {
        for (int i = 0; i < MAX_SPACES; i++) {
            if (occupied[i] && plates[i] == plate) {
                return i;
            }
        }
        return -1;
    }

    // Automatic choice: the CHEAPEST free space this plan may use.
    // Check Standard first, then Premium, then VIP.
    int chooseSpace(int plan) {
        for (int t = 0; t < TIER_COUNT; t++) {
            if (t == 2 && !planVip[plan]) {
                continue;   // VIP is only for Gold and Platinum
            }
            for (int i = 0; i < MAX_SPACES; i++) {
                if (!occupied[i] && tiers[i] == t) {
                    return i;
                }
            }
        }
        return -1;
    }

    // Price per hour of space i for a type of vehicle.
    double hourlyRate(int i, string type) {
        return baseRate(type) * tierMultiplier[tiers[i]];
    }

    // Hours on the clock (minimum 1).
    int hoursParked(int i) {
        int hours = clockHour - entryHour[i];
        if (hours < 1) {
            hours = 1;
        }
        return hours;
    }

    // Hours that are paid for (Platinum gets the first hour free).
    int chargedHours(int i, int plan) {
        int hours = hoursParked(i);
        if (planFreeHour[plan]) {
            hours = hours - 1;
        }
        return hours;
    }

    double subtotal(int i, int plan) {
        return chargedHours(i, plan) * hourlyRate(i, types[i]);
    }

    // Final price after the member discount.
    double feeNow(int i, int plan) {
        return subtotal(i, plan) * (1.0 - planDiscount[plan]);
    }

    // space = -1 means "choose automatically".
    string park(string plate, string type, int space, int plan) {
        if (type != "car" && type != "bike") {
            return "Invalid type. Use bike or car.";
        }
        if (findByPlate(plate) != -1) {
            return plate + " is already parked.";
        }
        if (space == -1) {
            space = chooseSpace(plan);
            if (space == -1) {
                return "No free space you can use right now.";
            }
        }
        if (space < 0 || space >= MAX_SPACES) {
            return "That space does not exist.";
        }
        if (occupied[space]) {
            return spaceLabel(space) + " is already taken.";
        }
        if (tiers[space] == 2 && !planVip[plan]) {
            return spaceLabel(space) + " is VIP: only Gold and Platinum members can park there.";
        }
        occupied[space] = true;
        plates[space] = plate;
        types[space] = type;
        entryHour[space] = clockHour;
        return plate + " parked in " + spaceLabel(space) + " at " + hourText(clockHour) + ".";
    }

    string remove(string plate, int plan) {
        int space = findByPlate(plate);
        if (space == -1) {
            return plate + " not found.";
        }
        double fee = feeNow(space, plan);
        totalCollected = totalCollected + fee;
        string message = plate + " left " + spaceLabel(space)
                         + " and paid " + moneyText(fee) + ".";
        occupied[space] = false;
        plates[space] = "";
        types[space] = "";
        entryHour[space] = 0;
        return message;
    }

    string search(string plate) {
        int space = findByPlate(plate);
        if (space == -1) {
            return plate + " not found.";
        }
        return plate + " is in " + spaceLabel(space) + " since " + hourText(entryHour[space]) + ".";
    }

    string advance(int n) {
        if (n < 1) {
            return "Please choose at least 1 hour.";
        }
        clockHour = clockHour + n;
        return "Clock moved forward to " + hourText(clockHour) + ".";
    }

    int countBusy() {
        int busy = 0;
        for (int i = 0; i < MAX_SPACES; i++) {
            if (occupied[i]) {
                busy++;
            }
        }
        return busy;
    }

    int countFreeInTier(int t) {
        int free = 0;
        for (int i = 0; i < MAX_SPACES; i++) {
            if (!occupied[i] && tiers[i] == t) {
                free++;
            }
        }
        return free;
    }

    bool   isOccupied(int i) { return occupied[i]; }
    string getPlate(int i)   { return plates[i]; }
    string getType(int i)    { return types[i]; }
    int    getEntry(int i)   { return entryHour[i]; }
    int    getTier(int i)    { return tiers[i]; }
    int    getClock()        { return clockHour; }
    double getTotal()        { return totalCollected; }
};

// =============================================================
//  PART 5 - Window settings: colours, layout, buttons, state
// =============================================================

const COLORREF NAVY      = RGB(20, 33, 61);
const COLORREF NAVY_SOFT = RGB(38, 55, 96);
const COLORREF YELLOW    = RGB(242, 183, 5);
const COLORREF CREAM     = RGB(244, 242, 236);
const COLORREF PANEL     = RGB(255, 255, 255);
const COLORREF BORDER    = RGB(226, 222, 210);
const COLORREF SOFT_GREY = RGB(238, 236, 230);
const COLORREF ASPHALT   = RGB(43, 52, 71);
const COLORREF SHADOW    = RGB(31, 38, 53);
const COLORREF LANE      = RGB(70, 82, 108);
const COLORREF FREE_TEXT = RGB(125, 139, 168);
const COLORREF CAR_BLUE  = RGB(47, 111, 219);
const COLORREF BIKE_GRN  = RGB(40, 150, 80);
const COLORREF RED       = RGB(214, 82, 52);
const COLORREF WHITE     = RGB(255, 255, 255);
const COLORREF BLACK     = RGB(0, 0, 0);
const COLORREF LIGHT     = RGB(225, 231, 242);
const COLORREF MUTED     = RGB(96, 106, 124);
const COLORREF GOLD      = RGB(150, 112, 0);
const COLORREF GLASS     = RGB(196, 218, 246);

// Colours for each zone (badge colour and free-space colour)
const COLORREF tierColor[TIER_COUNT] = {RGB(140, 154, 184), RGB(38, 176, 160), RGB(226, 172, 30)};
const COLORREF tierFree[TIER_COUNT]  = {RGB(55, 66, 90), RGB(36, 72, 82), RGB(74, 64, 40)};

// Colour for each membership plan
const COLORREF planColor[PLAN_COUNT] = {RGB(120, 130, 148), RGB(130, 145, 165),
                                        RGB(214, 158, 14), RGB(118, 92, 214)};

// Window size in "design pixels" (scaled to the screen's DPI)
const int WIN_W = 1280;
const int WIN_H = 760;
double scale = 1.0;

const int CARD_W = 150;
const int CARD_H = 200;

// Animation lengths (milliseconds)
const int ANIM_PARK    = 500;
const int ANIM_SELECT  = 1400;
const int ANIM_FLASH   = 900;
const int ANIM_RECEIPT = 380;
const int ANIM_TOAST   = 2800;
const int ANIM_TAB     = 300;

// Buttons, stored in parallel arrays. btnTab: -1 = always shown,
// 0 = only on the Parking tab, 1 = only on the Memberships tab.
const int BTN_TAB_PARK = 0, BTN_TAB_MEMBERS = 1, BTN_CAR = 2, BTN_BIKE = 3,
          BTN_AUTO = 4, BTN_PARK = 5, BTN_REMOVE = 6, BTN_SEARCH = 7,
          BTN_MINUS = 8, BTN_PLUS = 9, BTN_ADVANCE = 10, BTN_PLAN0 = 11,
          BTN_DROP_MEMBER = 15, BTN_ZONE0 = 16,   // BTN_ZONE0..2 = zone tabs
          BTN_APPLY_PLAN = 19;
const int BUTTON_COUNT = 20;
int btnX[BUTTON_COUNT]   = {892, 1068, 892, 1068, 1164, 892, 1010, 1128, 892, 1024, 1076,
                            892, 1068, 892, 1068, 1070, 36, 308, 580, 892};
int btnY[BUTTON_COUNT]   = {206, 206, 278, 278, 362, 420, 420, 420, 510, 510, 510,
                            278, 278, 402, 402, 528, 98, 98, 98, 528};
int btnW[BUTTON_COUNT]   = {172, 172, 172, 172, 68, 112, 112, 112, 40, 40, 164,
                            172, 172, 172, 172, 170, 256, 256, 256, 170};
int btnH[BUTTON_COUNT]   = {38, 38, 40, 40, 32, 44, 44, 44, 40, 40, 40,
                            116, 116, 116, 116, 36, 46, 46, 46, 36};
int btnTab[BUTTON_COUNT] = {-1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, -1, -1, -1, 1};
string btnText[BUTTON_COUNT] = {"Parking", "Memberships", "Car", "Bike", "Auto",
                                "Park", "Remove", "Search", "-", "+", "Advance",
                                "", "", "", "", "Remove membership", "", "", "", "Apply"};

// Activity feed (newest first). kind: 0 = info, 1 = success, 2 = error
const int LOG_SIZE = 3;
string logText[LOG_SIZE];
int    logKind[LOG_SIZE];
int    logCount = 0;

// The two objects that hold all the data
ParkingLot lot;
MemberList members;

// What the user is doing
int currentTab  = 0;
int pendingPlan = -1;     // plan card picked, waiting for Apply (-1 = none)
int viewTier    = 0;     // zone tab shown in the lot (0 Standard, 1 Premium, 2 VIP)
int chosenSpace = -1;     // free space picked by the user (-1 = automatic)
int selected    = -1;     // occupied space that is highlighted
int hoverCard   = -1;
int hoverButton = -1;
int hoverMember = -1;
int hoverRemoveX = -1;   // member row whose X is under the mouse
string vehicleType = "car";
int hoursStep = 1;
bool trackingMouse = false;

// Animation start times (0 = not running)
DWORD arriveStart[MAX_SPACES];
DWORD leaveStart[MAX_SPACES];
string leavePlate[MAX_SPACES];
string leaveType[MAX_SPACES];
DWORD selectStart = 0, clockFlash = 0, errorFlash = 0, logStart = 0;
DWORD pressStart = 0, toastStart = 0, tabStart = 0, planFlash = 0, lotTabStart = 0;
int   pressedButton = -1;
int   flashPlan = -1;
string toastText;
int    toastKind = 0;
double shownParking = 0;    // money shown in the top bar, counts up
double shownSales = 0;

// Receipt card data
bool  receiptOpen = false;
DWORD receiptStart = 0;
string rPlate, rType;
int    rSpace, rTier, rIn, rOut, rHours, rCharged, rPlan;
double rRate, rSubtotal, rDiscount, rFee;

// Fonts, the plate text box and its brush
HFONT titleFont, bigFont, plateFont, boldFont, normalFont, smallFont, inputFont;
HWND  hPlate;
HBRUSH fieldBrush;

HFONT makeFont(int size, int weight) {
    return CreateFontA(-size, 0, 0, 0, weight, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
}

// Card positions on a zone tab: 1-5 on top, 6-10 below the lane
int cardX(int i) { return 37 + (i % 5) * 162; }
int cardY(int i) {
    if (i % SPACES_PER_TIER < 5) {
        return 156;
    }
    return 412;
}

// Rows of the member list on the Memberships tab
const int MEMBER_ROWS = 4;
int memberRowY(int k) { return 598 + k * 31; }

// =============================================================
//  PART 6 - Animation helpers
// =============================================================

double progress(DWORD start, int duration, DWORD now) {
    if (start == 0) {
        return 1.0;
    }
    double t = (double)(now - start) / duration;
    if (t > 1.0) {
        t = 1.0;
    }
    return t;
}

bool running(DWORD start, int duration, DWORD now) {
    return start != 0 && now - start < (DWORD)duration;
}

// Fast at first, then slows down smoothly
double easeOut(double t) {
    double u = 1.0 - t;
    return 1.0 - u * u * u;
}

// Mix two colours: t = 0 gives c1, t = 1 gives c2
COLORREF blend(COLORREF c1, COLORREF c2, double t) {
    int r = GetRValue(c1) + (int)((GetRValue(c2) - GetRValue(c1)) * t);
    int g = GetGValue(c1) + (int)((GetGValue(c2) - GetGValue(c1)) * t);
    int b = GetBValue(c1) + (int)((GetBValue(c2) - GetBValue(c1)) * t);
    return RGB(r, g, b);
}

// Move a shown number smoothly toward the real number
double countUp(double shown, double target) {
    shown = shown + (target - shown) * 0.12;
    if (target - shown < 0.005) {
        shown = target;
    }
    return shown;
}

bool anythingAnimating(DWORD now) {
    for (int i = 0; i < MAX_SPACES; i++) {
        if (running(arriveStart[i], ANIM_PARK, now) || running(leaveStart[i], ANIM_PARK, now)) {
            return true;
        }
    }
    if (running(selectStart, ANIM_SELECT, now) || running(clockFlash, ANIM_FLASH, now) ||
        running(errorFlash, ANIM_FLASH, now) || running(logStart, ANIM_FLASH, now) ||
        running(receiptStart, ANIM_RECEIPT, now) || running(pressStart, 150, now) ||
        running(toastStart, ANIM_TOAST, now) || running(tabStart, ANIM_TAB, now) ||
        running(lotTabStart, ANIM_TAB, now) ||
        running(planFlash, ANIM_FLASH, now)) {
        return true;
    }
    return shownParking != lot.getTotal() || shownSales != members.getSales();
}

// =============================================================
//  PART 7 - Drawing helpers
// =============================================================

void fillRect(HDC dc, int x, int y, int w, int h, COLORREF color) {
    RECT r = {x, y, x + w, y + h};
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(dc, &r, brush);
    DeleteObject(brush);
}

void roundBox(HDC dc, int x, int y, int w, int h, COLORREF fill,
              COLORREF border, int penWidth, int corner) {
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, penWidth, border);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    RoundRect(dc, x, y, x + w, y + h, corner, corner);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

void drawText(HDC dc, string text, int x, int y, int w, int h,
              HFONT font, COLORREF color, UINT align) {
    SelectObject(dc, font);
    SetTextColor(dc, color);
    RECT r = {x, y, x + w, y + h};
    DrawTextA(dc, text.c_str(), -1, &r, align | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
}

void drawWrapped(HDC dc, string text, int x, int y, int w, int h,
                 HFONT font, COLORREF color) {
    SelectObject(dc, font);
    SetTextColor(dc, color);
    RECT r = {x, y, x + w, y + h};
    DrawTextA(dc, text.c_str(), -1, &r, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
}

// A small coloured label with rounded ends
void pill(HDC dc, string text, int x, int y, int w, COLORREF fill, COLORREF textColor) {
    roundBox(dc, x, y, w, 22, fill, fill, 1, 22);
    drawText(dc, text, x, y, w, 22, smallFont, textColor, DT_CENTER);
}

// Top-down picture of a car or a bike
void drawVehicle(HDC dc, string type, int centerX, int top, COLORREF cardColor) {
    COLORREF body = blend(cardColor, WHITE, 0.22);
    if (type == "car") {
        roundBox(dc, centerX - 22, top, 44, 64, body, body, 1, 18);
        roundBox(dc, centerX - 16, top + 10, 32, 15, GLASS, GLASS, 1, 8);
        roundBox(dc, centerX - 16, top + 44, 32, 10, GLASS, GLASS, 1, 6);
    } else {
        roundBox(dc, centerX - 5, top + 4, 10, 58, body, body, 1, 10);
        roundBox(dc, centerX - 20, top + 16, 40, 6, NAVY, NAVY, 1, 6);
    }
}

// =============================================================
//  PART 8 - Drawing the parking lot
// =============================================================

void drawOccupiedContent(HDC dc, int i, int x, int y, string plate, string type,
                         COLORREF fill, bool leaving) {
    drawVehicle(dc, type, x + CARD_W / 2, y + 40, fill);
    drawText(dc, plate, x, y + 106, CARD_W, 26, plateFont, WHITE, DT_CENTER);
    if (leaving) {
        drawText(dc, "Leaving...", x, y + 140, CARD_W, 22, normalFont, LIGHT, DT_CENTER);
        return;
    }
    int plan = members.planOf(plate);
    string info = "In " + hourText(lot.getEntry(i)) + "   " + intToText(lot.hoursParked(i)) + " h";
    drawText(dc, info, x, y + 130, CARD_W, 20, normalFont, LIGHT, DT_CENTER);
    drawText(dc, "Due " + moneyText(lot.feeNow(i, plan)), x, y + 150, CARD_W, 24,
             boldFont, WHITE, DT_CENTER);
    if (plan > 0) {
        string tag = planName[plan] + "  -" + percentText(planDiscount[plan]);
        pill(dc, tag, x + 25, y + 174, CARD_W - 50, planColor[plan], WHITE);
    }
}

void drawFreeContent(HDC dc, int i, int x, int y) {
    int tier = lot.getTier(i);
    drawText(dc, "FREE", x, y + 50, CARD_W, 34, bigFont, FREE_TEXT, DT_CENTER);
    drawText(dc, "Car  " + moneyText(lot.hourlyRate(i, "car")) + "/h",
             x, y + 92, CARD_W, 22, normalFont, LIGHT, DT_CENTER);
    drawText(dc, "Bike  " + moneyText(lot.hourlyRate(i, "bike")) + "/h",
             x, y + 114, CARD_W, 22, normalFont, LIGHT, DT_CENTER);
    if (tier == 2) {
        drawText(dc, "Gold & Platinum", x, y + 142, CARD_W, 20, smallFont, tierColor[2], DT_CENTER);
    }
    if (i == chosenSpace) {
        pill(dc, "YOUR CHOICE", x + 25, y + 168, CARD_W - 50, YELLOW, NAVY);
    }
}

void drawCard(HDC dc, int i, int slideX, DWORD now) {
    int x = cardX(i) + slideX;
    int y = cardY(i);
    int tier = lot.getTier(i);
    bool hover = (i == hoverCard);
    if (hover) {
        y = y - 3;
    }
    int laneSide = (i % SPACES_PER_TIER < 5) ? 1 : -1;   // which way the lane is

    COLORREF fill = tierFree[tier];
    bool showVehicle = false;
    bool leaving = false;
    string plate, type;
    int offsetY = 0;

    if (lot.isOccupied(i)) {
        plate = lot.getPlate(i);
        type = lot.getType(i);
        COLORREF typeColor = (type == "car") ? CAR_BLUE : BIKE_GRN;
        double e = easeOut(progress(arriveStart[i], ANIM_PARK, now));
        fill = blend(tierFree[tier], typeColor, e);
        offsetY = (int)((1.0 - e) * 80) * laneSide;
        showVehicle = true;
    } else if (running(leaveStart[i], ANIM_PARK, now)) {
        plate = leavePlate[i];
        type = leaveType[i];
        COLORREF typeColor = (type == "car") ? CAR_BLUE : BIKE_GRN;
        double e = easeOut(progress(leaveStart[i], ANIM_PARK, now));
        fill = blend(typeColor, tierFree[tier], e);
        offsetY = (int)(e * 100) * laneSide;
        showVehicle = true;
        leaving = true;
    }
    if (hover) {
        fill = blend(fill, WHITE, 0.08);
    }

    roundBox(dc, x, y + 6, CARD_W, CARD_H, SHADOW, SHADOW, 1, 20);
    bool highlight = (i == selected) || (i == chosenSpace);
    if (highlight) {
        int border = 4;
        if (running(selectStart, ANIM_SELECT, now)) {
            border = 3 + (int)((now - selectStart) % 350) * 4 / 350;   // pulse
        }
        roundBox(dc, x, y, CARD_W, CARD_H, fill, YELLOW, border, 20);
    } else {
        roundBox(dc, x, y, CARD_W, CARD_H, fill, fill, 1, 20);
    }

    // Number and zone badge
    drawText(dc, intToText(i % SPACES_PER_TIER + 1), x + 14, y + 10, 40, 26, bigFont, YELLOW, DT_LEFT);
    int badgeW = (tier == 2) ? 44 : 84;
    pill(dc, tierName[tier], x + CARD_W - badgeW - 12, y + 12, badgeW,
         blend(tierColor[tier], BLACK, 0.25), WHITE);

    if (showVehicle) {
        SaveDC(dc);
        IntersectClipRect(dc, x + 3, y + 38, x + CARD_W - 3, y + CARD_H - 3);
        drawOccupiedContent(dc, i, x, y + offsetY, plate, type, fill, leaving);
        RestoreDC(dc, -1);
    } else {
        drawFreeContent(dc, i, x, y);
    }
}

// One zone tab above the lot, e.g. "Premium  x1.5 price  7 / 10 free"
void drawZoneTab(HDC dc, int tier) {
    int b = BTN_ZONE0 + tier;
    int x = btnX[b], y = btnY[b], w = btnW[b], h = btnH[b];
    bool active = (tier == viewTier);
    COLORREF fill = active ? blend(tierColor[tier], BLACK, 0.35) : tierFree[tier];
    if (b == hoverButton && !active) {
        fill = blend(fill, WHITE, 0.1);
    }
    COLORREF border = active ? tierColor[tier] : fill;
    roundBox(dc, x, y, w, h, fill, border, 2, 14);
    roundBox(dc, x + 14, y + 17, 12, 12, tierColor[tier], tierColor[tier], 1, 12);
    drawText(dc, tierTitle[tier], x + 34, y + 2, 120, 24, boldFont, WHITE, DT_LEFT);
    string info = multiplierText(tierMultiplier[tier]) + " price";
    if (tier == 2) {
        info = info + ", Gold & Platinum";
    }
    drawText(dc, info, x + 34, y + 22, w - 40, 20, smallFont, LIGHT, DT_LEFT);
    string freeText = intToText(lot.countFreeInTier(tier)) + " / "
                      + intToText(SPACES_PER_TIER) + " free";
    drawText(dc, freeText, x, y + 2, w - 14, 24, smallFont, tierColor[tier], DT_RIGHT);
}

// The message bubble that appears in the driving lane
void drawToast(HDC dc, DWORD now) {
    if (!running(toastStart, ANIM_TOAST, now)) {
        return;
    }
    DWORD age = now - toastStart;
    double appear = easeOut(progress(toastStart, 250, now));
    double fade = 0;
    if (age > (DWORD)(ANIM_TOAST - 400)) {
        fade = (age - (ANIM_TOAST - 400)) / 400.0;
    }
    COLORREF fill = NAVY;
    if (toastKind == 1) {
        fill = BIKE_GRN;
    } else if (toastKind == 2) {
        fill = RED;
    }
    fill = blend(fill, ASPHALT, fade);
    COLORREF textColor = blend(WHITE, ASPHALT, fade);
    int w = 640;
    int x = 116 + (int)((1.0 - appear) * -40);
    roundBox(dc, x, 363, w, 44, fill, fill, 1, 44);
    drawText(dc, toastText, x + 20, 363, w - 40, 44, boldFont, textColor, DT_CENTER);
}

// =============================================================
//  PART 9 - Drawing the right panel
// =============================================================

void drawButton(HDC dc, int b, DWORD now);

void drawPlanCard(HDC dc, int p, DWORD now) {
    int b = BTN_PLAN0 + p;
    int x = btnX[b], y = btnY[b], w = btnW[b], h = btnH[b];
    string plate = "";
    char buffer[32];
    GetWindowTextA(hPlate, buffer, 32);
    plate = buffer;
    bool current = (plate != "" && members.planOf(plate) == p);

    COLORREF fill = PANEL;
    if (b == hoverButton) {
        fill = blend(PANEL, planColor[p], 0.08);
    }
    bool picked = (p == pendingPlan);
    COLORREF border = current ? planColor[p] : BORDER;
    int penW = current ? 3 : 1;
    if (picked) {                       // picked cards get a yellow border
        border = YELLOW;
        penW = 4;
        fill = blend(PANEL, YELLOW, 0.10);
    }
    if (p == flashPlan && running(planFlash, ANIM_FLASH, now)) {
        fill = blend(blend(planColor[p], WHITE, 0.6), fill, progress(planFlash, ANIM_FLASH, now));
    }
    roundBox(dc, x, y, w, h, fill, border, penW, 16);

    drawText(dc, planName[p], x + 14, y + 8, w - 28, 26, bigFont, planColor[p], DT_LEFT);
    string price = "Free";
    if (p > 0) {
        price = moneyText(planPrice[p]) + " / month";
    }
    drawText(dc, price, x + 14, y + 34, w - 28, 22, boldFont, NAVY, DT_LEFT);

    string line1 = "No discount";
    if (p > 0) {
        line1 = percentText(planDiscount[p]) + " off parking";
    }
    string line2 = "Standard & Premium";
    if (planVip[p]) {
        line2 = "VIP spaces unlocked";
    }
    drawText(dc, line1, x + 14, y + 56, w - 28, 20, normalFont, MUTED, DT_LEFT);
    drawText(dc, line2, x + 14, y + 74, w - 28, 20, normalFont, MUTED, DT_LEFT);
    if (planFreeHour[p]) {
        drawText(dc, "First hour free", x + 14, y + 92, w - 28, 20, normalFont, MUTED, DT_LEFT);
    }
    if (picked) {
        pill(dc, "SELECTED", x + w - 86, y + 11, 74, YELLOW, NAVY);
    } else if (current) {
        pill(dc, "CURRENT", x + w - 80, y + 11, 68, planColor[p], WHITE);
    }
}

void drawButton(HDC dc, int b, DWORD now) {
    if (b >= BTN_PLAN0 && b < BTN_PLAN0 + PLAN_COUNT) {
        drawPlanCard(dc, b - BTN_PLAN0, now);
        return;
    }
    if (b >= BTN_ZONE0 && b < BTN_ZONE0 + TIER_COUNT) {
        drawZoneTab(dc, b - BTN_ZONE0);
        return;
    }
    COLORREF fill = NAVY;
    COLORREF text = WHITE;
    bool outline = false;
    string label = btnText[b];

    if (b == BTN_PARK) {
        fill = BIKE_GRN;
    } else if (b == BTN_REMOVE || b == BTN_DROP_MEMBER) {
        fill = RED;
    } else if (b == BTN_APPLY_PLAN) {
        if (pendingPlan == -1) {        // nothing picked yet: grey button
            fill = SOFT_GREY;
            text = MUTED;
            label = "Pick a plan";
        } else {
            fill = BIKE_GRN;
            label = "Apply " + planName[pendingPlan];
        }
    } else if (b == BTN_ADVANCE) {
        fill = YELLOW;
        text = NAVY;
    } else if (b == BTN_MINUS || b == BTN_PLUS || b == BTN_AUTO) {
        fill = SOFT_GREY;
        text = NAVY;
    } else if (b == BTN_CAR || b == BTN_BIKE) {
        bool chosen = (b == BTN_CAR && vehicleType == "car") ||
                      (b == BTN_BIKE && vehicleType == "bike");
        if (!chosen) {
            fill = PANEL;
            text = NAVY;
            outline = true;
        }
    } else if (b == BTN_TAB_PARK || b == BTN_TAB_MEMBERS) {
        bool active = (b == BTN_TAB_PARK && currentTab == 0) ||
                      (b == BTN_TAB_MEMBERS && currentTab == 1);
        if (!active) {
            fill = SOFT_GREY;
            text = NAVY;
        }
    }
    if (b == hoverButton) {
        fill = blend(fill, WHITE, 0.15);
    }
    if (b == pressedButton && running(pressStart, 150, now)) {
        fill = blend(fill, BLACK, 0.2);
    }
    COLORREF border = outline ? BORDER : fill;
    roundBox(dc, btnX[b], btnY[b], btnW[b], btnH[b], fill, border, 1, 12);
    drawText(dc, label, btnX[b], btnY[b], btnW[b], btnH[b], boldFont, text, DT_CENTER);
}

// Membership status of the typed plate, shown under the plate box
void drawMemberStatus(HDC dc, DWORD now) {
    char buffer[32];
    GetWindowTextA(hPlate, buffer, 32);
    string plate = buffer;
    if (plate == "") {
        drawText(dc, "Type a plate to see its membership.", 892, 176, 348, 22,
                 normalFont, MUTED, DT_LEFT);
        return;
    }
    int p = members.planOf(plate);
    COLORREF fill = planColor[p];
    if (running(planFlash, ANIM_FLASH, now)) {
        fill = blend(YELLOW, fill, progress(planFlash, ANIM_FLASH, now));
    }
    string label = (p == 0) ? "GUEST" : planName[p] + " MEMBER";
    pill(dc, label, 892, 176, 120, fill, WHITE);
    string perks = "full price, no VIP";
    if (p > 0) {
        perks = percentText(planDiscount[p]) + " off";
        if (planVip[p]) {
            perks = perks + ", VIP access";
        }
        if (planFreeHour[p]) {
            perks = perks + ", 1st hour free";
        }
    }
    drawText(dc, perks, 1020, 176, 220, 22, normalFont, MUTED, DT_LEFT);
}

void drawParkingTab(HDC dc, int dx, DWORD now) {
    drawText(dc, "VEHICLE", 892 + dx, 254, 200, 18, smallFont, GOLD, DT_LEFT);
    drawText(dc, "SPACE", 892 + dx, 332, 200, 18, smallFont, GOLD, DT_LEFT);

    // The space chooser box
    roundBox(dc, 892 + dx, 354, 348, 48, SOFT_GREY, SOFT_GREY, 1, 12);
    if (chosenSpace == -1) {
        drawText(dc, "Automatic", 906 + dx, 356, 240, 24, boldFont, NAVY, DT_LEFT);
        drawText(dc, "cheapest free space you can use", 906 + dx, 376, 250, 22,
                 normalFont, MUTED, DT_LEFT);
    } else {
        int t = lot.getTier(chosenSpace);
        drawText(dc, spaceLabel(chosenSpace) + "  -  " + multiplierText(tierMultiplier[t]) + " price",
                 906 + dx, 356, 250, 24, boldFont, NAVY, DT_LEFT);
        drawText(dc, vehicleType + "  " + moneyText(lot.hourlyRate(chosenSpace, vehicleType)) + " / hour",
                 906 + dx, 376, 250, 22, normalFont, MUTED, DT_LEFT);
    }

    for (int b = 0; b < BUTTON_COUNT; b++) {
        if (btnTab[b] == 0 && !(b == BTN_AUTO && chosenSpace == -1)) {
            drawButton(dc, b, now);
        }
    }

    fillRect(dc, 892, 478, 348, 1, BORDER);
    drawText(dc, "CLOCK", 892 + dx, 488, 200, 18, smallFont, GOLD, DT_LEFT);
    drawText(dc, intToText(hoursStep) + " h", 932 + dx, 510, 92, 40, bigFont, NAVY, DT_CENTER);

    fillRect(dc, 892, 564, 348, 1, BORDER);
    drawText(dc, "ACTIVITY", 892 + dx, 574, 200, 18, smallFont, GOLD, DT_LEFT);
    for (int k = 0; k < logCount; k++) {
        int rowY = 598 + k * 48;
        int slide = 0;
        COLORREF rowFill = PANEL;
        if (k == 0) {
            double e = easeOut(progress(logStart, ANIM_FLASH, now));
            slide = (int)((1.0 - e) * 24);
            rowFill = blend(RGB(255, 243, 205), PANEL, e);
        }
        roundBox(dc, 886, rowY - 2, 360, 46, rowFill, rowFill, 1, 12);
        COLORREF dot = CAR_BLUE;
        if (logKind[k] == 1) {
            dot = BIKE_GRN;
        } else if (logKind[k] == 2) {
            dot = RED;
        }
        roundBox(dc, 896 + slide, rowY + 6, 10, 10, dot, dot, 1, 10);
        drawWrapped(dc, logText[k], 914 + slide, rowY, 322, 42, normalFont,
                    (k == 0) ? NAVY : MUTED);
    }
}

void drawMembersTab(HDC dc, int dx, DWORD now) {
    drawText(dc, "PLANS  (pick one, then press Apply)", 892 + dx, 254, 348, 18,
             smallFont, GOLD, DT_LEFT);
    for (int b = BTN_PLAN0; b < BTN_PLAN0 + PLAN_COUNT; b++) {
        drawButton(dc, b, now);
    }
    drawButton(dc, BTN_APPLY_PLAN, now);
    drawButton(dc, BTN_DROP_MEMBER, now);

    int count = members.getCount();
    drawText(dc, "MEMBERS  (" + intToText(count) + ")   click a name to use it, X to remove",
             892 + dx, 576, 348, 18, smallFont, GOLD, DT_LEFT);
    if (count == 0) {
        drawText(dc, "No members yet.", 892 + dx, 598, 348, 24, normalFont, MUTED, DT_LEFT);
        return;
    }
    int shown = count;
    if (shown > MEMBER_ROWS) {
        shown = MEMBER_ROWS;
    }
    for (int k = 0; k < shown; k++) {
        int rowY = memberRowY(k);
        COLORREF rowFill = (k == hoverMember) ? SOFT_GREY : PANEL;
        roundBox(dc, 886, rowY, 360, 28, rowFill, rowFill, 1, 10);
        string plate = members.getPlate(k);
        drawText(dc, plate, 900 + dx, rowY, 110, 28, boldFont, NAVY, DT_LEFT);
        string where = "not parked";
        int space = lot.findByPlate(plate);
        if (space != -1) {
            where = "in " + spaceLabel(space);
        }
        drawText(dc, where, 1000 + dx, rowY, 110, 28, normalFont, MUTED, DT_LEFT);
        int p = members.getPlan(k);
        pill(dc, planName[p], 1112 + dx, rowY + 3, 84, planColor[p], WHITE);
        // The X button that removes this membership
        COLORREF xFill = (k == hoverRemoveX) ? RED : SOFT_GREY;
        COLORREF xText = (k == hoverRemoveX) ? WHITE : RED;
        roundBox(dc, 1206 + dx, rowY + 2, 26, 24, xFill, xFill, 1, 10);
        drawText(dc, "X", 1206 + dx, rowY + 2, 26, 24, boldFont, xText, DT_CENTER);
    }
    if (count > shown) {
        drawText(dc, "+ " + intToText(count - shown) + " more", 900, 724, 200, 20,
                 normalFont, MUTED, DT_LEFT);
    }
    if (count > shown) {
        drawText(dc, "+ " + intToText(count - shown) + " more", 900, 718, 200, 20,
                 normalFont, MUTED, DT_LEFT);
    }
}

// The receipt that slides up after a vehicle leaves
void drawReceipt(HDC dc, DWORD now) {
    double e = easeOut(progress(receiptStart, ANIM_RECEIPT, now));
    int w = 400;
    int h = 448;
    int x = 236;
    int y = 108 + (int)((1.0 - e) * 120);

    roundBox(dc, x + 6, y + 10, w, h, SHADOW, SHADOW, 1, 22);
    roundBox(dc, x, y, w, h, PANEL, PANEL, 1, 22);
    roundBox(dc, x, y, w, 60, NAVY, NAVY, 1, 22);
    fillRect(dc, x, y + 40, w, 20, NAVY);
    drawText(dc, "RECEIPT", x + 24, y + 10, 200, 40, bigFont, YELLOW, DT_LEFT);
    drawText(dc, spaceLabel(rSpace), x, y + 10, w - 24, 40,
             boldFont, LIGHT, DT_RIGHT);

    string hoursValue = intToText(rHours);
    if (rCharged != rHours) {
        hoursValue = intToText(rHours) + "  (1 free)";
    }
    string labels[8] = {"Plate", "Vehicle", "In / Out", "Hours", "Rate",
                        "Subtotal", "Member", "Discount"};
    string values[8] = {rPlate, rType, hourText(rIn) + "  -  " + hourText(rOut), hoursValue,
                        moneyText(baseRate(rType)) + " " + multiplierText(tierMultiplier[rTier])
                            + " = " + moneyText(rRate) + "/h",
                        moneyText(rSubtotal), planName[rPlan],
                        "-" + moneyText(rDiscount) + "  (" + percentText(planDiscount[rPlan]) + ")"};
    for (int k = 0; k < 8; k++) {
        int rowY = y + 72 + k * 30;
        drawText(dc, labels[k], x + 24, rowY, 120, 28, normalFont, MUTED, DT_LEFT);
        COLORREF valueColor = NAVY;
        if (k == 6) {
            valueColor = planColor[rPlan];
        }
        drawText(dc, values[k], x + 120, rowY, w - 144, 28, boldFont, valueColor, DT_RIGHT);
    }
    fillRect(dc, x + 24, y + 318, w - 48, 1, BORDER);
    drawText(dc, "TOTAL", x + 24, y + 328, 120, 48, bigFont, NAVY, DT_LEFT);
    drawText(dc, moneyText(rFee), x + 120, y + 328, w - 144, 48, titleFont, BIKE_GRN, DT_RIGHT);
    drawText(dc, "Click anywhere to close", x, y + 400, w, 28, normalFont, MUTED, DT_CENTER);
}

void drawStat(HDC dc, int x, int w, string label, string value,
              COLORREF fill, COLORREF valueColor) {
    roundBox(dc, x, 12, w, 52, fill, fill, 1, 14);
    drawText(dc, label, x + 14, 16, w - 28, 16, smallFont, FREE_TEXT, DT_LEFT);
    drawText(dc, value, x + 14, 32, w - 28, 28, bigFont, valueColor, DT_LEFT);
}

void drawEverything(HDC dc, DWORD now) {
    SetBkMode(dc, TRANSPARENT);
    fillRect(dc, 0, 0, WIN_W, WIN_H, CREAM);

    // ---- Top bar ----
    fillRect(dc, 0, 0, WIN_W, 76, NAVY);
    roundBox(dc, 22, 16, 44, 44, YELLOW, YELLOW, 1, 12);
    drawText(dc, "P", 22, 16, 44, 44, titleFont, NAVY, DT_CENTER);
    drawText(dc, "Smart Parking", 80, 10, 300, 32, titleFont, WHITE, DT_LEFT);
    pill(dc, "PRO", 262, 18, 46, YELLOW, NAVY);
    drawText(dc, "Zones, your own space & memberships", 81, 42, 400, 22, normalFont, FREE_TEXT, DT_LEFT);

    double flash = progress(clockFlash, ANIM_FLASH, now);
    drawStat(dc, 600, 130, "CLOCK", hourText(lot.getClock()),
             blend(YELLOW, NAVY_SOFT, flash), blend(NAVY, YELLOW, flash));
    int busy = lot.countBusy();
    drawStat(dc, 742, 140, "OCCUPIED", intToText(busy) + " / " + intToText(MAX_SPACES), NAVY_SOFT, YELLOW);
    fillRect(dc, 830, 44, 40, 6, LANE);
    fillRect(dc, 830, 44, busy * 40 / MAX_SPACES, 6, YELLOW);
    drawStat(dc, 894, 170, "PARKING INCOME", moneyText(shownParking), NAVY_SOFT, YELLOW);
    drawStat(dc, 1076, 184, "MEMBERSHIPS (" + intToText(members.getCount()) + ")",
             moneyText(shownSales), NAVY_SOFT, YELLOW);

    // ---- Parking lot ----
    roundBox(dc, 20, 88, 832, 540, ASPHALT, ASPHALT, 1, 26);
    for (int t = 0; t < TIER_COUNT; t++) {
        drawZoneTab(dc, t);
    }
    for (int x = 44; x < 840; x = x + 76) {
        fillRect(dc, x, 382, 40, 5, LANE);
    }
    // Only the 10 spaces of the chosen zone tab. They slide in when
    // you switch tabs, so we cut the drawing off at the lot's edge.
    int slideX = (int)((1.0 - easeOut(progress(lotTabStart, ANIM_TAB, now))) * 60);
    int first = viewTier * SPACES_PER_TIER;
    SaveDC(dc);
    IntersectClipRect(dc, 20, 150, 852, 628);
    for (int i = first; i < first + SPACES_PER_TIER; i++) {
        drawCard(dc, i, slideX, now);
    }
    RestoreDC(dc, -1);
    drawToast(dc, now);

    // ---- Help under the lot ----
    roundBox(dc, 24, 656, 14, 14, CAR_BLUE, CAR_BLUE, 1, 14);
    drawText(dc, "Car  $1.50/h base", 46, 652, 150, 22, normalFont, NAVY, DT_LEFT);
    roundBox(dc, 200, 656, 14, 14, BIKE_GRN, BIKE_GRN, 1, 14);
    drawText(dc, "Bike  $0.50/h base", 222, 652, 150, 22, normalFont, NAVY, DT_LEFT);
    drawText(dc, "Zone price = base x zone (VIP x2.0, Premium x1.5, Standard x1.0)",
             380, 652, 470, 22, normalFont, MUTED, DT_RIGHT);
    drawText(dc, "Click a free space to choose it. With no choice, Park picks the cheapest space you can use.",
             24, 680, 830, 22, normalFont, MUTED, DT_LEFT);
    drawText(dc, "Due = what it pays if it leaves now (min. 1 hour, after member discount).",
             24, 706, 830, 22, normalFont, MUTED, DT_LEFT);

    // ---- Right panel ----
    roundBox(dc, 872, 88, 388, 660, PANEL, BORDER, 1, 26);
    drawText(dc, "PLATE NUMBER", 892, 102, 200, 18, smallFont, GOLD, DT_LEFT);
    COLORREF fieldBorder = blend(RED, BORDER, progress(errorFlash, ANIM_FLASH, now));
    roundBox(dc, 892, 124, 348, 44, PANEL, fieldBorder, 2, 12);
    drawMemberStatus(dc, now);
    drawButton(dc, BTN_TAB_PARK, now);
    drawButton(dc, BTN_TAB_MEMBERS, now);

    // The tab content slides in a little when you switch tabs
    int dx = (int)((1.0 - easeOut(progress(tabStart, ANIM_TAB, now))) * 30);
    if (currentTab == 0) {
        drawParkingTab(dc, dx, now);
    } else {
        drawMembersTab(dc, dx, now);
    }

    if (receiptOpen) {
        drawReceipt(dc, now);
    }
}

// =============================================================
//  PART 10 - User actions
// =============================================================

void addMessage(string text, int kind) {
    for (int k = LOG_SIZE - 1; k > 0; k--) {
        logText[k] = logText[k - 1];
        logKind[k] = logKind[k - 1];
    }
    logText[0] = text;
    logKind[0] = kind;
    if (logCount < LOG_SIZE) {
        logCount++;
    }
    logStart = GetTickCount();
    toastText = text;          // also show it in the lane
    toastKind = kind;
    toastStart = GetTickCount();
}

void showError(string text) {
    addMessage(text, 2);
    errorFlash = GetTickCount();
}

string getPlate() {
    char buffer[32];
    GetWindowTextA(hPlate, buffer, 32);
    return string(buffer);
}

int cardAt(int mx, int my) {
    int first = viewTier * SPACES_PER_TIER;   // only the zone tab on screen
    for (int i = first; i < first + SPACES_PER_TIER; i++) {
        if (mx >= cardX(i) && mx <= cardX(i) + CARD_W &&
            my >= cardY(i) && my <= cardY(i) + CARD_H) {
            return i;
        }
    }
    return -1;
}

bool buttonVisible(int b) {
    if (b == BTN_AUTO && chosenSpace == -1) {
        return false;
    }
    return btnTab[b] == -1 || btnTab[b] == currentTab;
}

int buttonAt(int mx, int my) {
    for (int b = 0; b < BUTTON_COUNT; b++) {
        if (buttonVisible(b) && mx >= btnX[b] && mx <= btnX[b] + btnW[b] &&
            my >= btnY[b] && my <= btnY[b] + btnH[b]) {
            return b;
        }
    }
    return -1;
}

int memberRowAt(int mx, int my) {
    if (currentTab != 1) {
        return -1;
    }
    for (int k = 0; k < members.getCount() && k < MEMBER_ROWS; k++) {
        int rowY = memberRowY(k);
        if (mx >= 886 && mx <= 1246 && my >= rowY && my <= rowY + 28) {
            return k;
        }
    }
    return -1;
}

// Is the mouse on the X of a member row? Returns the row or -1.
int memberRemoveAt(int mx, int my) {
    int k = memberRowAt(mx, my);
    if (k != -1 && mx >= 1206 && mx <= 1232) {
        return k;
    }
    return -1;
}

// Switch the lot to another zone tab (with a slide animation).
void showZone(int tier) {
    if (tier != viewTier) {
        viewTier = tier;
        lotTabStart = GetTickCount();
    }
}

void doPark() {
    string plate = getPlate();
    if (plate == "") {
        showError("Type a plate number first.");
        return;
    }
    int plan = members.planOf(plate);
    bool wasParked = (lot.findByPlate(plate) != -1);
    string message = lot.park(plate, vehicleType, chosenSpace, plan);
    int space = lot.findByPlate(plate);
    if (!wasParked && space != -1) {
        addMessage(message, 1);
        arriveStart[space] = GetTickCount();
        leaveStart[space] = 0;
        showZone(lot.getTier(space));   // show the zone the vehicle went to
        selected = space;
        selectStart = GetTickCount();
        chosenSpace = -1;
        SetWindowTextA(hPlate, "");
    } else {
        showError(message);
    }
}

void doRemove() {
    string plate = getPlate();
    if (plate == "") {
        showError("Type or click a plate first.");
        return;
    }
    int space = lot.findByPlate(plate);
    if (space == -1) {
        showError(plate + " not found.");
        return;
    }
    // Save everything the receipt needs BEFORE the space is cleared
    int plan = members.planOf(plate);
    rPlate = plate;
    rType = lot.getType(space);
    rSpace = space;
    rTier = lot.getTier(space);
    rIn = lot.getEntry(space);
    rOut = lot.getClock();
    rHours = lot.hoursParked(space);
    rCharged = lot.chargedHours(space, plan);
    rPlan = plan;
    rRate = lot.hourlyRate(space, rType);
    rSubtotal = lot.subtotal(space, plan);
    rFee = lot.feeNow(space, plan);
    rDiscount = rSubtotal - rFee;
    leavePlate[space] = plate;
    leaveType[space] = rType;

    addMessage(lot.remove(plate, plan), 1);
    leaveStart[space] = GetTickCount();
    arriveStart[space] = 0;
    receiptOpen = true;
    receiptStart = GetTickCount();
    selected = -1;
    SetWindowTextA(hPlate, "");
}

void doSearch() {
    string plate = getPlate();
    if (plate == "") {
        showError("Type a plate number first.");
        return;
    }
    int space = lot.findByPlate(plate);
    if (space == -1) {
        showError(lot.search(plate));
        selected = -1;
    } else {
        addMessage(lot.search(plate), 0);
        showZone(lot.getTier(space));
        selected = space;
        chosenSpace = -1;
        selectStart = GetTickCount();
    }
}

void doJoin(int plan) {
    string plate = getPlate();
    if (plate == "") {
        showError("Type a plate number first, then choose a plan.");
        return;
    }
    int before = members.planOf(plate);
    string message = members.join(plate, plan);
    if (members.planOf(plate) != before) {
        addMessage(message, 1);
        flashPlan = plan;
        planFlash = GetTickCount();
        pendingPlan = -1;           // done: clear the selection
    } else {
        showError(message);
    }
}

void doRemoveMember(string plate) {
    if (plate == "") {
        showError("Type a member's plate first, or click X in the list.");
        return;
    }
    if (members.find(plate) == -1) {
        showError(plate + " is not a member.");
        return;
    }
    flashPlan = members.planOf(plate);
    planFlash = GetTickCount();
    addMessage(members.removeMember(plate), 1);
}

// Clicking a member's name: put the plate in the box and show the vehicle
void useMember(int k) {
    string plate = members.getPlate(k);
    SetWindowTextA(hPlate, plate.c_str());
    int space = lot.findByPlate(plate);
    if (space != -1) {
        showZone(lot.getTier(space));
        selected = space;
        selectStart = GetTickCount();
    }
}

// Clicking a plan card only SELECTS it. Click it again to unselect.
void pickPlan(int plan) {
    if (pendingPlan == plan) {
        pendingPlan = -1;
        addMessage(planName[plan] + " unselected.", 0);
        return;
    }
    pendingPlan = plan;
    if (plan == 0) {
        addMessage("Guest selected. Press Apply to end the membership.", 0);
    } else {
        addMessage(planName[plan] + " selected (" + moneyText(planPrice[plan])
                   + " / month). Press Apply to confirm.", 0);
    }
}

void pressButton(int b) {
    pressedButton = b;
    pressStart = GetTickCount();
    if (b == BTN_TAB_PARK || b == BTN_TAB_MEMBERS) {
        int tab = (b == BTN_TAB_PARK) ? 0 : 1;
        if (tab != currentTab) {
            currentTab = tab;
            tabStart = GetTickCount();
        }
    } else if (b == BTN_CAR) {
        vehicleType = "car";
    } else if (b == BTN_BIKE) {
        vehicleType = "bike";
    } else if (b == BTN_AUTO) {
        chosenSpace = -1;
    } else if (b == BTN_PARK) {
        doPark();
    } else if (b == BTN_REMOVE) {
        doRemove();
    } else if (b == BTN_SEARCH) {
        doSearch();
    } else if (b == BTN_MINUS && hoursStep > 1) {
        hoursStep--;
    } else if (b == BTN_PLUS && hoursStep < 12) {
        hoursStep++;
    } else if (b == BTN_ADVANCE) {
        addMessage(lot.advance(hoursStep), 0);
        clockFlash = GetTickCount();
    } else if (b >= BTN_PLAN0 && b < BTN_PLAN0 + PLAN_COUNT) {
        pickPlan(b - BTN_PLAN0);           // step 1: only select the plan
    } else if (b == BTN_APPLY_PLAN) {
        if (pendingPlan == -1) {           // step 2: apply it to the plate
            showError("Pick a plan first, then press Apply.");
        } else {
            doJoin(pendingPlan);
        }
    } else if (b == BTN_DROP_MEMBER) {
        doRemoveMember(getPlate());
        pendingPlan = -1;
    } else if (b >= BTN_ZONE0 && b < BTN_ZONE0 + TIER_COUNT) {
        showZone(b - BTN_ZONE0);
    }
}

// Clicking a space: choose a free one, or select a parked vehicle.
void clickSpace(int i) {
    if (lot.isOccupied(i)) {
        selected = i;
        chosenSpace = -1;
        selectStart = GetTickCount();
        string plate = lot.getPlate(i);
        SetWindowTextA(hPlate, plate.c_str());
        int plan = members.planOf(plate);
        addMessage(spaceLabel(i) + ": " + plate + ", due now "
                   + moneyText(lot.feeNow(i, plan)) + ".", 0);
        return;
    }
    selected = -1;
    if (chosenSpace == i) {
        chosenSpace = -1;
        addMessage("Back to automatic space choice.", 0);
        return;
    }
    chosenSpace = i;
    selectStart = GetTickCount();
    currentTab = 0;
    string note = spaceLabel(i) + " chosen. Press Park.";
    if (lot.getTier(i) == 2) {
        note = spaceLabel(i) + " chosen (Gold & Platinum only).";
    }
    addMessage(note, 0);
}

// =============================================================
//  PART 11 - Window procedure: Windows calls this for every event
// =============================================================
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CREATE) {
        // The only real text box. Its position is scaled for the screen.
        hPlate = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_UPPERCASE,
                               (int)(906 * scale), (int)(134 * scale),
                               (int)(320 * scale), (int)(26 * scale),
                               hwnd, NULL, GetModuleHandle(NULL), NULL);
        SendMessageA(hPlate, WM_SETFONT, (WPARAM)inputFont, TRUE);
        SendMessageA(hPlate, EM_LIMITTEXT, 10, 0);
        SetFocus(hPlate);
        addMessage("Welcome! Pick a zone tab, click a free space, then press Park.", 0);
        SetTimer(hwnd, 1, 15, NULL);
        return 0;

    } else if (msg == WM_TIMER) {
        shownParking = countUp(shownParking, lot.getTotal());
        shownSales = countUp(shownSales, members.getSales());
        if (anythingAnimating(GetTickCount())) {
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;

    } else if (msg == WM_PAINT) {
        // Double buffering: draw on a hidden picture, then copy it at once.
        // The drawing uses design pixels; SetViewportExtEx scales them up.
        PAINTSTRUCT ps;
        HDC screen = BeginPaint(hwnd, &ps);
        RECT client;
        GetClientRect(hwnd, &client);
        HDC memory = CreateCompatibleDC(screen);
        HBITMAP picture = CreateCompatibleBitmap(screen, client.right, client.bottom);
        HGDIOBJ old = SelectObject(memory, picture);
        SetMapMode(memory, MM_ANISOTROPIC);
        SetWindowExtEx(memory, WIN_W, WIN_H, NULL);
        SetViewportExtEx(memory, client.right, client.bottom, NULL);
        drawEverything(memory, GetTickCount());
        SetMapMode(memory, MM_TEXT);
        BitBlt(screen, 0, 0, client.right, client.bottom, memory, 0, 0, SRCCOPY);
        SelectObject(memory, old);
        DeleteObject(picture);
        DeleteDC(memory);
        EndPaint(hwnd, &ps);
        return 0;

    } else if (msg == WM_ERASEBKGND) {
        return 1;

    } else if (msg == WM_COMMAND) {
        // The plate text changed: redraw so the member status updates
        if (HIWORD(wParam) == EN_CHANGE) {
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;

    } else if (msg == WM_MOUSEMOVE) {
        int mx = (int)((short)LOWORD(lParam) / scale);
        int my = (int)((short)HIWORD(lParam) / scale);
        int newCard = cardAt(mx, my);
        int newButton = buttonAt(mx, my);
        int newMember = memberRowAt(mx, my);
        int newRemoveX = memberRemoveAt(mx, my);
        if (newCard != hoverCard || newButton != hoverButton || newMember != hoverMember ||
            newRemoveX != hoverRemoveX) {
            hoverCard = newCard;
            hoverButton = newButton;
            hoverMember = newMember;
            hoverRemoveX = newRemoveX;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        if (!trackingMouse) {
            TRACKMOUSEEVENT tme;
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            tme.dwHoverTime = 0;
            TrackMouseEvent(&tme);
            trackingMouse = true;
        }
        return 0;

    } else if (msg == WM_MOUSELEAVE) {
        trackingMouse = false;
        hoverCard = -1;
        hoverButton = -1;
        hoverMember = -1;
        hoverRemoveX = -1;
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    } else if (msg == WM_SETCURSOR) {
        if (LOWORD(lParam) == HTCLIENT &&
            (hoverCard != -1 || hoverButton != -1 || hoverMember != -1)) {
            SetCursor(LoadCursor(NULL, IDC_HAND));
            return TRUE;
        }

    } else if (msg == WM_LBUTTONDOWN) {
        int mx = (int)((short)LOWORD(lParam) / scale);
        int my = (int)((short)HIWORD(lParam) / scale);
        bool wasReceipt = receiptOpen;
        receiptOpen = false;                 // any click closes the receipt
        int b = buttonAt(mx, my);
        int i = cardAt(mx, my);
        int m = memberRowAt(mx, my);
        int removeRow = memberRemoveAt(mx, my);
        if (b != -1) {
            pressButton(b);
        } else if (removeRow != -1) {
            doRemoveMember(members.getPlate(removeRow));
            hoverRemoveX = -1;
            hoverMember = -1;
        } else if (m != -1) {
            useMember(m);
        } else if (i != -1 && !wasReceipt) {
            clickSpace(i);
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    } else if (msg == WM_CTLCOLOREDIT) {
        HDC dc = (HDC)wParam;
        SetBkColor(dc, PANEL);
        SetTextColor(dc, NAVY);
        return (LRESULT)fieldBrush;

    } else if (msg == WM_DESTROY) {
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

// =============================================================
//  PART 12 - WinMain: the GUI version of main()
// =============================================================

// Tell Windows we draw sharp text ourselves at any screen scaling.
// (Looked up by name because older compilers do not know it.)
void makeDpiAware() {
    typedef BOOL (WINAPI *DpiFunction)(void);
    HMODULE user32 = GetModuleHandleA("user32.dll");
    DpiFunction setAware = (DpiFunction)(void (*)(void))GetProcAddress(user32, "SetProcessDPIAware");
    if (setAware != NULL) {
        setAware();
    }
    HDC screen = GetDC(NULL);
    scale = GetDeviceCaps(screen, LOGPIXELSX) / 96.0;   // 120 dpi -> 1.25
    ReleaseDC(NULL, screen);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int showCommand) {
    makeDpiAware();

    titleFont  = makeFont(26, FW_BOLD);
    bigFont    = makeFont(20, FW_BOLD);
    plateFont  = makeFont(19, FW_BOLD);
    boldFont   = makeFont(15, FW_BOLD);
    normalFont = makeFont(15, FW_NORMAL);
    smallFont  = makeFont(12, FW_BOLD);
    inputFont  = makeFont((int)(20 * scale), FW_BOLD);   // the text box is not scaled for us
    fieldBrush = CreateSolidBrush(PANEL);

    for (int i = 0; i < MAX_SPACES; i++) {
        arriveStart[i] = 0;
        leaveStart[i] = 0;
    }

    WNDCLASSA wc = {};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = "SmartParkingProWindow";
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;
    RECT size = {0, 0, (int)(WIN_W * scale), (int)(WIN_H * scale)};
    AdjustWindowRect(&size, style, FALSE);
    HWND hwnd = CreateWindowA("SmartParkingProWindow", "Smart Parking PRO",
                              style, CW_USEDEFAULT, CW_USEDEFAULT,
                              size.right - size.left, size.bottom - size.top,
                              NULL, NULL, hInstance, NULL);
    ShowWindow(hwnd, showCommand);

    MSG message;
    while (GetMessage(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    DeleteObject(fieldBrush);
    return 0;
}
