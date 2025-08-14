/******************************************************************************************************************
 ** Kevin Randolph
 ** CS-300-12333-M01
 ** Professor Norman Lippincott
 ** Assignment 7-1 Project Two Submisson
 ** Last updated: 8/14/2025
 ** ProjectTwo.cpp
 **
 ** This program loads course data from a file into a hash table and lets the user view all courses in
 ** alphanumeric order or search for a specific course to see its title and prerequisites. It includes input
 ** validation, warnings for duplicates or mismatch prerequisites, and color coded output to make the information
 ** easier to read.
 ******************************************************************************************************************/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cctype>
#include <limits>

using namespace std;

// color codes for menu/output formatting
#define COLOR_YELLOW "\033[33m"
#define COLOR_RED    "\033[31m"
#define COLOR_GREEN  "\033[32m"
#define COLOR_BLUE   "\033[36m"  
#define COLOR_RESET  "\033[0m"

// struct to store course information
struct Course {
    string number;
    string title;
    vector<string> prereqs;
};

// hash table to store courses & quick lookup by course number
static unordered_map<string, Course> g_coursesByNumber;
// set to keep track of course numbers (used for prereq validation)
static unordered_set<string> g_allCourseNums;

// remove spaces, tabs & newlines from both ends of a string
static string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    if (a == string::npos) return "";
    return s.substr(a, (b - a + 1));
}

// normalize course codes. remove spaces, dashes & make uppercase
static string normalizeCode(string s) {
    string out;
    for (char c : s) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (isspace(uc) || c == '-') continue;
        out += static_cast<char>(toupper(uc));
    }
    return out;
}

// split CSV line into tokens
static vector<string> splitCSV(const string& line) {
    vector<string> tokens;
    string token;
    stringstream ss(line);
    while (getline(ss, token, ',')) {
        tokens.push_back(trim(token));
    }
    return tokens;
}

// break course number into letters & digits for sorting
static pair<string, int> parseAlnumKey(const string& courseNumIn) {
    string courseNum = normalizeCode(courseNumIn);
    string letters, digits;
    for (char c : courseNum) {
        if (isalpha(static_cast<unsigned char>(c))) letters += c;
        else if (isdigit(static_cast<unsigned char>(c))) digits += c;
    }
    int num = digits.empty() ? 0 : stoi(digits);
    return { letters, num };
}

// comparison for sorting courses in alphanumeric order
static bool courseLess(const Course& a, const Course& b) {
    auto ka = parseAlnumKey(a.number);
    auto kb = parseAlnumKey(b.number);
    if (ka.first != kb.first) return ka.first < kb.first;
    if (ka.second != kb.second) return ka.second < kb.second;
    return a.number < b.number;
}

// try to open file, also check with .txt & .csv extensions if no extension given
static bool tryOpenInput(const string& rawName, ifstream& in) {
    in.open(rawName);
    if (in.good()) return true;
    in.close();
    if (rawName.find('.') != string::npos) return false;
    string withTxt = rawName + ".txt";
    in.open(withTxt);
    if (in.good()) return true;
    in.close();
    string withCsv = rawName + ".csv";
    in.open(withCsv);
    if (in.good()) return true;
    in.close();
    return false;
}

// load courses from file into hash table
static bool loadCourses(const string& filenameRaw) {
    g_coursesByNumber.clear();
    g_allCourseNums.clear();

    ifstream in;
    if (!tryOpenInput(filenameRaw, in)) {
        cout << COLOR_RED << "Error: could not open file '" << filenameRaw
            << "'. (Tried with and without .txt/.csv)" << COLOR_RESET << "\n";
        return false;
    }

    string line;
    size_t lineNum = 0, loaded = 0;
    while (getline(in, line)) {
        ++lineNum;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        line = trim(line);
        if (line.empty()) continue;

        // parse line into tokens
        vector<string> toks = splitCSV(line);
        if (toks.size() < 2) {
            cout << COLOR_RED << "Format error (line " << lineNum
                << "): need at least course number and title." << COLOR_RESET << "\n";
            continue;
        }

        Course c;
        c.number = normalizeCode(toks[0]);
        c.title = toks[1];
        for (size_t i = 2; i < toks.size(); ++i) {
            if (!toks[i].empty()) c.prereqs.push_back(normalizeCode(toks[i]));
        }

        // overwrite duplicate if it exists
        if (g_coursesByNumber.find(c.number) != g_coursesByNumber.end()) {
            cout << COLOR_YELLOW << "Warning: duplicate course '" << c.number
                << "' encountered; overwriting previous entry." << COLOR_RESET << "\n";
        }
        g_allCourseNums.insert(c.number);
        g_coursesByNumber[c.number] = std::move(c);
        ++loaded;
    }
    in.close();

    // check prereqs against listed courses
    for (const auto& kv : g_coursesByNumber) {
        const auto& course = kv.second;
        for (const auto& pre : course.prereqs) {
            if (!g_allCourseNums.count(pre)) {
                cout << COLOR_YELLOW << "Warning: prerequisite '" << pre
                    << "' for " << course.number << " not found in file." << COLOR_RESET << "\n";
            }
        }
    }

    cout << COLOR_YELLOW << loaded << " courses loaded." << COLOR_RESET << "\n";
    return loaded > 0;
}

// print courses in sorted order
static void printSortedCourseList() {
    if (g_coursesByNumber.empty()) {
        cout << COLOR_RED << "No courses loaded. Please load data first (option 1)." << COLOR_RESET << "\n";
        return;
    }
    vector<Course> v;
    v.reserve(g_coursesByNumber.size());
    for (const auto& kv : g_coursesByNumber) v.push_back(kv.second);
    sort(v.begin(), v.end(), courseLess);

    for (const auto& c : v) {
        cout << COLOR_GREEN << c.number << ": " << c.title << COLOR_RESET << "\n";
    }
}

// print info for a course (title & prereqs)
static void printCourseInfo(string queryRaw) {
    string query = normalizeCode(trim(queryRaw));
    if (query.empty()) {
        cout << COLOR_RED << "Please enter a course number." << COLOR_RESET << "\n";
        return;
    }
    auto it = g_coursesByNumber.find(query);
    if (it == g_coursesByNumber.end()) {
        cout << COLOR_RED << "Course not found: " << query << COLOR_RESET << "\n";
        return;
    }
    const Course& c = it->second;

    cout << COLOR_GREEN << c.number << ": " << c.title << COLOR_RESET << "\n";
    if (c.prereqs.empty()) {
        cout << COLOR_BLUE << "  Prerequisites:" << COLOR_RESET << " None\n";
    }
    else {
        cout << COLOR_BLUE << "  Prerequisites:" << COLOR_RESET << "\n";
        for (const auto& code : c.prereqs) {
            auto jt = g_coursesByNumber.find(code);
            if (jt != g_coursesByNumber.end()) {
                cout << COLOR_GREEN << "    " << code << ": " << jt->second.title << COLOR_RESET << "\n";
            }
            else {
                cout << COLOR_RED << "    " << code << " (missing)" << COLOR_RESET << "\n";
            }
        }
    }
}

// main program w/menu options
static void runMenu() {
    bool loaded = false;
    const string defaultName = "CS 300 ABCU_Advising_Program_Input";

    while (true) {
        // display main menu
        cout << COLOR_YELLOW
            << "\n====== Welcome To ABCUs Course Planner ======\n"
            << "1. Load course data from file\n"
            << "2. Print all courses\n"
            << "3. Print a course and applicable prerequisites\n"
            << "9. Exit\n"
            << COLOR_RESET
            << "Enter option: ";
        int opt;
        if (!(cin >> opt)) {
            cout << COLOR_RED << "Invalid input. Please enter a number." << COLOR_RESET << "\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        if (opt == 1) {
            // prompt for filename, with default fallback 
            cout << "Enter file name (press Enter for default: " << defaultName << "): ";
            string filename;
            getline(cin, filename);
            filename = trim(filename);
            if (filename.empty()) filename = defaultName;
            loaded = loadCourses(filename);
        }
        else if (opt == 2) {
            if (!loaded) {
                cout << COLOR_RED << "Please load data first using option 1." << COLOR_RESET << "\n";
                continue;
            }
            printSortedCourseList();
        }
        else if (opt == 3) {
            if (!loaded) {
                cout << COLOR_RED << "Please load data first using option 1." << COLOR_RESET << "\n";
                continue;
            }
            cout << "Enter course number (ex: CSCI200 or MATH201): ";
            string q;
            getline(cin, q);
            if (trim(q).empty()) {
                cout << COLOR_RED << "Please enter a course number." << COLOR_RESET << "\n";
                continue;
            }
            printCourseInfo(q);
        }
        else if (opt == 9) {
            cout << COLOR_YELLOW << "Goodbye." << COLOR_RESET << "\n";
            break;
        }
        else {
            cout << COLOR_RED << "Invalid option. Please choose 1, 2, 3, or 9." << COLOR_RESET << "\n";
        }
    }
}

int main() {
    runMenu();
    return 0;
}




