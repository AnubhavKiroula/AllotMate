#include <iostream>
#include "MatchingEngine.h"
#include "AllocationException.h"
#include <utility>
#include <vector>
#include <algorithm>

void MatchingEngine::loadData(
    const std::vector<Student>& inputStudents,
    const std::vector<Elective>& inputElectives
){
    //Build fresh indexes and preference data.
    HashTable<int, Student> newStudentIndex;
    HashTable<int, Elective> newElectiveIndex;
    SparseMatrix newPrefMatrix;

    HashTable<int, bool> studentIds;
    HashTable<int, bool> electiveIds;

    std::vector<Student> newStudents = inputStudents;
    std::vector<Elective> newElectives = inputElectives;

    //Validate and index electives.
    for (const Elective& elective : newElectives){
        if(electiveIds.contains(elective.getId())){
            throw AllocationException::InvalidPreferenceException(
                "Duplicate elective ID."
            );
        }

        electiveIds.insert(elective.getId(), true);
        newElectiveIndex.insert(elective.getId(), elective);
    }

    //Validate and index students.
    for(Student& student : newStudents){
        if(studentIds.contains(student.getId())){
            throw AllocationException::InvalidPreferenceException(
                "Duplicate student ID."
            );
        }

        studentIds.insert(student.getId(), true);
        student.setStatus(Status::UNMATCHED);

        const std::vector<int>& preferences= student.getPreferenceList();

        for(std::size_t i=0; i < preferences.size(); ++i){
            int electiveId = preferences[i];

            if(!electiveIds.contains(electiveId)){
                throw AllocationException::InvalidPreferenceException(
                    "Student preference references an unkown elective ID."
                );
            }
            newPrefMatrix.insert(
                student.getId(),
                electiveId,
                static_cast<int>(i) + 1
            );
        }

        newStudentIndex.insert(student.getId(), student);
    }

    // Replace the engine's data only after validation succeeds.
    studentIndex = std::move(newStudentIndex);
    electiveIndex = std::move(newElectiveIndex);
    prefMatrix = std::move(newPrefMatrix);
    students = std::move(newStudents);
    electives = std::move(newElectives);

}

void MatchingEngine::runGaleShapley() {

    Stack<Student> pendingStudents;
    unmatchedStack = Stack<Student>();

    // Reset student allocation status and proposal positions.
    for (Student& student : students) {
        student.setStatus(Status::UNMATCHED);
        studentIndex.find(student.getId()).setStatus(Status::UNMATCHED);
        nextPreferenceIndex.insert(student.getId(), 0);
    }

    // Reset elective seat counts and assignment lists.
    for (Elective& elective : electives) {
        elective.resetSeats();
        electiveIndex.find(elective.getId()).resetSeats();

        electiveAssignments.insert(
            elective.getId(),
            std::vector<int>{}
        );
    }

    // Every student starts unmatched and may make a proposal.
    for (const Student& student : students) {
        pendingStudents.push(student);
    }

    while (!pendingStudents.empty()) {
        Student poppedStudent = pendingStudents.top();
        pendingStudents.pop();

        int studentId = poppedStudent.getId();
        Student currentStudent = studentIndex.find(studentId);

        const std::vector<int>& preferences = currentStudent.getPreferenceList();

        int& nextIndex = nextPreferenceIndex.find(studentId);

        // No preferences left: the student remains unmatched.
        if (nextIndex >= static_cast<int>(preferences.size())) {
            unmatchedStack.push(currentStudent);
            continue; // Prevent an exhausted student from being processed forever.
        }

        int electiveId = preferences[nextIndex];
        ++nextIndex;

        Elective& elective = electiveIndex.find(electiveId);
        std::vector<int>& assignedStudents = electiveAssignments.find(electiveId);

        // Skip electives with no available capacity.
        if (elective.getCapacity() == 0) {
            if (nextIndex < static_cast<int>(preferences.size())) {
                pendingStudents.push(currentStudent);
            }
            else{
                unmatchedStack.push(currentStudent);
            }
            continue;
        }

        if (elective.hasSeat()) {
            elective.allocateSeat();
            assignedStudents.push_back(studentId);
            studentIndex.find(studentId).setStatus(Status::ALLOTED);
            continue;
        }

        // The elective is full. Rank its current students plus.
        // the new applicant using the custom PriorityQueue.
        PriorityQueue<Student> ranking;

        for (int assignedId : assignedStudents) {
            ranking.push(studentIndex.find(assignedId));
        }

        ranking.push(currentStudent);

        // A max-heap pops highest priority first, so the last
        // student popped has the lowest priority.
        Student lowestPriorityStudent = ranking.top();

        while (!ranking.empty()){
            lowestPriorityStudent = ranking.top();
            ranking.pop();
        }

        int lowestPriorityId = lowestPriorityStudent.getId();

        // The new applicant is lowest-ranked: reject them.
        if(lowestPriorityId == studentId){
            if(nextIndex < static_cast<int>(preferences.size())){
                pendingStudents.push(currentStudent);
            }
            else{
                unmatchedStack.push(currentStudent);
            }
            continue;
        }

        // Otherwise, replace the lowest-ranked assigned student.
        auto position = std::find(
            assignedStudents.begin(),
            assignedStudents.end(),
            lowestPriorityId
        );

        if(position != assignedStudents.end()){
            *position = studentId;
        }

        Student& displacedStudent = studentIndex.find(lowestPriorityId);

        displacedStudent.setStatus(Status::UNMATCHED);
        studentIndex.find(studentId).setStatus(Status::ALLOTED);

        // The elective remains full, so its seat count is unchanged.
        // The displaced student gets another chance if preferences remain.
        int& displacedNextIndex = nextPreferenceIndex.find(lowestPriorityId);

        const std::vector<int>& displacedPreferences = displacedStudent.getPreferenceList();

        if(displacedNextIndex < static_cast<int>(displacedPreferences.size())){
            pendingStudents.push(displacedStudent);
        }
        else{
            unmatchedStack.push(displacedStudent);
        }
    }

    // Synchronize public result vectors with indexed objects.
    for (Student& student : students) {
        student = studentIndex.find(student.getId());
    }

    for (Elective& elective : electives) {
        elective = electiveIndex.find(elective.getId());
    }
}

int MatchingEngine::getAssignedElectiveId(int studentId) const{
    for(const Elective& elective : electives){
        const std::vector<int>& assignedStudents = electiveAssignments.find(elective.getId());

        for(int assignedId : assignedStudents){
            if(assignedId == studentId){
                return elective.getId();
            }
        }
    }
    return -1; // Student is unmatched.
}

int MatchingEngine::detectBlockingPairs() const {
    int blockingPairCount = 0;

    for(const Student& student : students){
        const int studentId = student.getId();
        const int currentElectiveId = getAssignedElectiveId(studentId);

        const std::vector<int>& preferences = student.getPreferenceList();

        //check electives the student prefers over their current allocation.
        for(int preferredElectiveId : preferences){
            if(preferredElectiveId == currentElectiveId) break;

            const Elective& elective = electiveIndex.find(preferredElectiveId);

            // A zero-capacity elective cannot form a blocking pair.
            if(elective.getCapacity()==0){
                continue;
            }

            // A Student prefers any listed elective to remaining unmatched.
            if(currentElectiveId == -1 && elective.hasSeat()){
                std::cout << "Blocking Pair: Student " << studentId << ", Elective " << preferredElectiveId << "\n";
                ++blockingPairCount;
                continue;
            }

            // An elective with an available seat can accept this student.
            if(elective.hasSeat()){
                std::cout << "Blocking Pair: Student " << studentId << ", Elective " << preferredElectiveId << "\n";
                ++blockingPairCount;
                continue;
            }

            // If the elective is full, check whether it prefers this
            // student over at least one currently assigned student.
            const std::vector<int>& assignedStudents = electiveAssignments.find(preferredElectiveId);

            for(int assignedId : assignedStudents){
                const Student& assignedStudent = studentIndex.find(assignedId);

                if(assignedStudent < student){
                    std::cout << "Blocking Pair: Student " << studentId << ", Elective " << preferredElectiveId << "\n";
                    ++blockingPairCount;
                    break;
                }
            }
        }
    }
    std::cout << "Total Blocking Pairs: " << blockingPairCount << '\n';
    return blockingPairCount;
}

void MatchingEngine::generateFairnessReport() {
    int totalStudents = static_cast<int>(students.size());
    int unmatchedCount = 0;
    int firstChoiceCount = 0;
    int totalAssignedRank = 0;
    int matchedCount = 0;

    for(const Student& student : students){
        int assignedElectiveId = getAssignedElectiveId(student.getId());

        //Count students who did not receive an elective.
        if(assignedElectiveId == -1){
            ++unmatchedCount;
            continue;
        }

        const std::vector<int>& preferences = student.getPreferenceList();

        // Find the rank of the elective the student received.
        for(std::size_t i = 0; i < preferences.size(); ++i){
            if(preferences[i] == assignedElectiveId){
                int rank = static_cast<int>(i) + 1;
                totalAssignedRank += rank;
                ++matchedCount;

                if(rank == 1){
                    ++firstChoiceCount;
                }

                break;
            }
        }
    }
    double averageRank = matchedCount > 0 ? (static_cast<double>(totalAssignedRank) / matchedCount) : 0.0;
    
    double firstChoicePercentage = totalStudents > 0 ? (static_cast<double>(firstChoiceCount) / totalStudents) * 100.0 : 0.0;

    std::cout << "\n===== Fairness Report =====\n";
    std::cout << "Total students: " << totalStudents << '\n';
    std::cout << "Matched students: " << matchedCount << '\n';
    std::cout << "Unmatched students: " << unmatchedCount << '\n';
    std::cout << "Students receiving first choice: "
              << firstChoiceCount << '\n';
    std::cout << "First-choice percentage: "
              << firstChoicePercentage << "%\n";
    std::cout << "Average preference rank received: "
              << averageRank << '\n';
    std::cout << "===========================\n";
}

const std::vector<Student>& MatchingEngine::getStudents() const {
    return students;
}

const std::vector<Elective>& MatchingEngine::getElectives() const{
    return electives;
}