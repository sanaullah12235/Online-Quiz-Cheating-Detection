#include <iostream>
#include <fstream>
#include <ctime>
#include <cstdlib>
#include <string>


using namespace std;


struct Question {
    int id;
    string text;
    string options[4];
    int correctOption;
    Question* next;
};

struct TimeNode {
    int qID;
    double timeSpent;
    TimeNode* next;
};

// Custom Stack to track Time per Question (for Cheating Detection)
class TimingStack {
    TimeNode* top;
public:
    TimingStack() : top(NULL) {}
    ~TimingStack() {
        while (top) {
            TimeNode* temp = top;
            top = top->next;
            delete temp;
        }
    }
    void push(int id, double t) {
        TimeNode* newNode = new TimeNode{id, t, top};
        top = newNode;
    }
    void displayAll() {
        TimeNode* temp = top;
        cout << "\n--- Time Analysis (Question by Question) ---\n";
        while (temp) {
            cout << "Question ID " << temp->qID << ": " << temp->timeSpent << " seconds" << endl;
            temp = temp->next;
        }
    }
    double getLastTime() { return top ? top->timeSpent : 0; }
};

// ==========================================
// 2. FILE AND DATA MANAGEMENT CLASS
// ==========================================

class QuizSystem {
private:
    Question* head;
    int questionCount;
    const string qFile = "question_bank.txt";
    const string rFile = "student_results.txt";

public:
    QuizSystem() : head(NULL), questionCount(0) {
        loadQuestionsFromFile();
    }

    // Manual Question Entry
    void addQuestion(string txt, string opts[4], int correct, bool save = true) {
        Question* newNode = new Question;
        newNode->id = ++questionCount;
        newNode->text = txt;
        for (int i = 0; i < 4; i++) newNode->options[i] = opts[i];
        newNode->correctOption = correct;
        newNode->next = NULL;

        if (!head) {
            head = newNode;
        } else {
            Question* temp = head;
            while (temp->next) temp = temp->next;
            temp->next = newNode;
        }
        if (save) saveQuestionsToFile();
    }

    void editQuestion(int id) {
        Question* temp = head;
        while (temp && temp->id != id) temp = temp->next;
        if (temp) {
            cout << "Current Question: " << temp->text << endl;
            cout << "Enter New Question Text: ";
            cin.ignore();
            getline(cin, temp->text);
            for (int i = 0; i < 4; i++) {
                cout << "New Option " << i + 1 << ": ";
                getline(cin, temp->options[i]);
            }
            cout << "New Correct Option (1-4): ";
            cin >> temp->correctOption;
            saveQuestionsToFile();
            cout << "Question updated successfully!\n";
        } else {
            cout << "Question ID not found.\n";
        }
    }

    void deleteQuestion(int id) {
        if (!head) return;
        if (head->id == id) {
            Question* temp = head;
            head = head->next;
            delete temp;
        } else {
            Question* curr = head;
            while (curr->next && curr->next->id != id) curr = curr->next;
            if (curr->next) {
                Question* temp = curr->next;
                curr->next = temp->next;
                delete temp;
            }
        }
        // Re-indexing IDs
        Question* temp = head;
        int newID = 1;
        while (temp) {
            temp->id = newID++;
            temp = temp->next;
        }
        questionCount = newID - 1;
        saveQuestionsToFile();
    }

    void saveQuestionsToFile() {
        ofstream fout(qFile.c_str());
        Question* temp = head;
        while (temp) {
            fout << temp->text << "|" << temp->options[0] << "|" << temp->options[1] << "|"
                 << temp->options[2] << "|" << temp->options[3] << "|" << temp->correctOption << endl;
            temp = temp->next;
        }
        fout.close();
    }

    void loadQuestionsFromFile() {
        ifstream fin(qFile.c_str());
        if (!fin) return;
        string line;
        while (getline(fin, line)) {
            // Simplified parsing for file format: text|o1|o2|o3|o4|correct
            size_t pos = 0;
            string parts[6];
            int i = 0;
            while ((pos = line.find('|')) != string::npos && i < 5) {
                parts[i++] = line.substr(0, pos);
                line.erase(0, pos + 1);
            }
            parts[5] = line; // Last part is the correct answer
            string opts[4] = {parts[1], parts[2], parts[3], parts[4]};
            addQuestion(parts[0], opts, atoi(parts[5].c_str()), false);
        }
        fin.close();
    }

    void addDummyData() {
        if (questionCount > 0) return;
        string o1[4] = {"O(1)", "O(n)", "O(log n)", "O(n^2)"};
        addQuestion("Search complexity of Binary Search Tree (Average)?", o1, 3);
        string o2[4] = {"LIFO", "FIFO", "Random", "None"};
        addQuestion("What principle does a Stack follow?", o2, 1);
        string o3[4] = {"Array", "Linked List", "Tree", "Stack"};
        addQuestion("Which is a non-linear data structure?", o3, 3);
        string o4[4] = {"1", "2", "3", "Many"};
        addQuestion("How many pointers does a Singly Linked List node have?", o4, 1);
    }

    // ==========================================
    // 3. STUDENT MODULE
    // ==========================================

    void startQuiz(string name, string reg) {
        if (!head) {
            cout << "No questions available. Contact Teacher.\n";
            return;
        }

        TimingStack tStack;
        int score = 0;
        int cheatingFlags = 0;
        time_t startTime = time(0);
        const int TOTAL_TIME_LIMIT = 180; // 3 Minutes

        cout << "\n--- Welcome " << name << " (" << reg << ") ---" << endl;
        cout << "Rule: 180 seconds total. Avoid extremely fast answers (Cheating Detection).\n";

        Question* curr = head;
        while (curr) {
            time_t now = time(0);
            if (difftime(now, startTime) >= TOTAL_TIME_LIMIT) {
                cout << "\n[!] TIME OVER!\n";
                break;
            }

            cout << "\n" << curr->id << ". " << curr->text << endl;
            for (int i = 0; i < 4; i++) cout << "   " << i + 1 << ") " << curr->options[i] << endl;

            time_t qStart = time(0);
            int ans;
            cout << "Your Answer: ";
            if (!(cin >> ans)) { // Prevention of infinite loop on invalid char
                cin.clear();
                cin.ignore(1000, '\n');
                ans = -1;
            }
            time_t qEnd = time(0);

            double duration = difftime(qEnd, qStart);
            tStack.push(curr->id, duration);

            // Cheating Detection Logic
            if (duration < 1.5) cheatingFlags++; 
            if (ans == curr->correctOption) score++;

            curr = curr->next;
        }

        double totalTaken = difftime(time(0), startTime);
        string status = (cheatingFlags > 3) ? "SUSPECTED" : "CLEAN";

        // Display Result to Student
        cout << "\n========== RESULT ==========\n";
        cout << "Student: " << name << endl;
        cout << "Total Marks: " << score << "/" << questionCount << endl;
        cout << "Time Taken: " << totalTaken << " seconds" << endl;
        tStack.displayAll();
        cout << "Cheating Status: " << status << endl;
        cout << "============================\n";

        // Store in File
        ofstream fout(rFile.c_str(), ios::app);
        fout << "Reg:" << reg << " | Name:" << name << " | Score:" << score 
             << " | Status:" << status << " | Time:" << totalTaken << "s" << endl;
        fout.close();
    }

    void viewResults() {
        ifstream fin(rFile.c_str());
        string line;
        cout << "\n--- REGISTERED STUDENT RESULTS ---\n";
        if (!fin) cout << "No results found.\n";
        while (getline(fin, line)) cout << line << endl;
        fin.close();
    }
};

// ==========================================
// 4. MAIN INTERFACE
// ==========================================

void clearBuffer() {
    cin.clear();
    cin.ignore(1000, '\n');
}

int main() {
    QuizSystem system;
    system.addDummyData();
    int choice;

    while (true) {
        cout << "\n====================================";
        cout << "\n     DSA QUIZ MANAGEMENT SYSTEM";
        cout << "\n====================================";
        cout << "\n1. Login as Student";
        cout << "\n2. Login as Teacher";
        cout << "\n3. Exit";
        cout << "\nChoice: ";
        
        if (!(cin >> choice)) {
            clearBuffer();
            continue;
        }

        if (choice == 1) {
            string name, reg;
            cout << "Enter Name: "; cin.ignore(); getline(cin, name);
            cout << "Enter Registration Number: "; getline(cin, reg);
            system.startQuiz(name, reg);

        } else if (choice == 2) {
            string pass;
            cout << "Enter Teacher Password: ";
            cin >> pass;
            if (pass == "admin123") {
                int tChoice;
                do {
                    cout << "\n--- TEACHER MODULE ---";
                    cout << "\n1. View All Questions\n2. Add Question\n3. Edit Question";
                    cout << "\n4. Delete Question\n5. View Student Results\n6. Logout\nChoice: ";
                    cin >> tChoice;
                    if (tChoice == 1) {
                        system.saveQuestionsToFile(); // Ensure list is saved
                        ifstream fin("question_bank.txt");
                        string line;
                        while(getline(fin, line)) cout << line << endl;
                    } else if (tChoice == 2) {
                        string t, o[4]; int c;
                        cout << "Question: "; cin.ignore(); getline(cin, t);
                        for(int i=0; i<4; i++) { cout << "Opt "<<i+1<<": "; getline(cin, o[i]); }
                        cout << "Correct (1-4): "; cin >> c;
                        system.addQuestion(t, o, c);
                    } else if (tChoice == 3) {
                        int id; cout << "Enter ID to Edit: "; cin >> id;
                        system.editQuestion(id);
                    } else if (tChoice == 4) {
                        int id; cout << "Enter ID to Delete: "; cin >> id;
                        system.deleteQuestion(id);
                    } else if (tChoice == 5) {
                        system.viewResults();
                    }
                } while (tChoice != 6);
            } else {
                cout << "Invalid Password!\n";
            }

        } else if (choice == 3) {
            break;
        }
    }
    return 0;
}