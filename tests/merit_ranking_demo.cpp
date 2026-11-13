#include "MeritRankingReport.h"

int main() {
    std::vector<Student> students = {
        Student(104, "Dev", 9.0f),
        Student(101, "Asha", 9.0f),
        Student(103, "Meera", 9.0f),
        Student(102, "Ravi", 8.5f)
    };
    MeritRankingReport::print(students);
    return 0;
}