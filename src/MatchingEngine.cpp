#include "MatchingEngine.h"
#include "AllocationException.h"
#include <stdexcept>
#include <utility>
#include <vector>

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
        for(int& assignedId : assignedStudents){
            if(assignedId == lowestPriorityId){
                assignedId = studentId;
                break;
            }
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

std::vector<int> MatchingEngine::getAssignedElectiveIds() const {
    // Turn electiveAssignments (elective -> students) around once, so we
    // don't search every elective again for every student.
    HashTable<int, int> electiveOfStudent;
    for(const Elective& elective : electives){
        for(int studentId : electiveAssignments.find(elective.getId())){
            electiveOfStudent.insert(studentId, elective.getId());
        }
    }

    std::vector<int> assigned;
    for(const Student& student : students){
        const int studentId = student.getId();
        assigned.push_back(electiveOfStudent.contains(studentId) ? electiveOfStudent.find(studentId) : -1);
    }
    return assigned;
}

std::vector<Elective> MatchingEngine::buildWaitlists() const {
    // Fresh copies (empty waitlists), plus elective id -> position in the vector.
    std::vector<Elective> waitlists;
    HashTable<int, int> positionOf;
    for(const Elective& elective : electives){
        positionOf.insert(elective.getId(), static_cast<int>(waitlists.size()));
        waitlists.push_back(Elective(elective.getId(), elective.getName(), elective.getCapacity()));
    }

    const std::vector<int> assigned = getAssignedElectiveIds();

    for(std::size_t i = 0; i < students.size(); ++i){
        // Same walk as countBlockingPairs: stop at the elective the student got.
        for(int preferredElectiveId : students[i].getPreferenceList()){
            if(preferredElectiveId == assigned[i]) break;

            // The AVL tree keeps each waitlist ordered by priority (CGPA, then lower ID).
            waitlists[positionOf.find(preferredElectiveId)].addToWaitlist(students[i]);
        }
    }
    return waitlists;
}

int MatchingEngine::countBlockingPairs(
    const std::vector<Student>& students,
    const std::vector<Elective>& electives,
    const std::vector<int>& assignedElectiveIds
){
    if(assignedElectiveIds.size() != students.size()){
        throw std::invalid_argument("countBlockingPairs: one assignment is needed per student.");
    }

    // Per elective: capacity, seats taken, and the index (in students) of
    // the lowest-priority student it holds (-1 while it holds nobody).
    HashTable<int, int> capacity;
    HashTable<int, int> seatsTaken;
    HashTable<int, int> weakestHolder;

    for(const Elective& elective : electives){
        capacity.insert(elective.getId(), elective.getCapacity());
        seatsTaken.insert(elective.getId(), 0);
        weakestHolder.insert(elective.getId(), -1);
    }

    for(std::size_t i = 0; i < students.size(); ++i){
        const int electiveId = assignedElectiveIds[i];
        if(electiveId == -1) continue;

        ++seatsTaken.find(electiveId);
        int& weakest = weakestHolder.find(electiveId);
        if(weakest == -1 || students[i] < students[weakest]){
            weakest = static_cast<int>(i);
        }
    }

    int blockingPairCount = 0;

    for(std::size_t i = 0; i < students.size(); ++i){
        const Student& student = students[i];

        // Check only electives the student prefers over their current allocation.
        // An unmatched student (-1) prefers every elective on their list.
        for(int preferredElectiveId : student.getPreferenceList()){
            if(preferredElectiveId == assignedElectiveIds[i]) break;

            // An elective with an available seat can accept this student.
            const bool hasFreeSeat = seatsTaken.find(preferredElectiveId) < capacity.find(preferredElectiveId);

            // A full elective blocks if it prefers this student over the
            // lowest-priority student it holds.
            const int weakest = weakestHolder.find(preferredElectiveId);
            const bool prefersThisStudent = weakest != -1 && students[weakest] < student;

            // A zero-capacity elective has no free seat and nobody to replace,
            // so it can never form a blocking pair (SRS #3).
            if(hasFreeSeat || prefersThisStudent){
                ++blockingPairCount;
            }
        }
    }
    return blockingPairCount;
}

FairnessReport MatchingEngine::buildFairnessReport(
    const std::vector<Student>& students,
    const std::vector<Elective>& electives,
    const std::vector<int>& assignedElectiveIds
){
    if(assignedElectiveIds.size() != students.size()){
        throw std::invalid_argument("buildFairnessReport: one assignment is needed per student.");
    }

    FairnessReport report;
    report.totalStudents = static_cast<int>(students.size());
    int totalAssignedRank = 0;

    for(std::size_t i = 0; i < students.size(); ++i){
        const int assignedElectiveId = assignedElectiveIds[i];

        //Count students who did not receive an elective.
        if(assignedElectiveId == -1){
            ++report.unmatched;
            continue;
        }
        ++report.matched;

        // Find the rank of the elective the student received (1 = first choice).
        const std::vector<int>& preferences = students[i].getPreferenceList();
        for(std::size_t rank = 0; rank < preferences.size(); ++rank){
            if(preferences[rank] == assignedElectiveId){
                totalAssignedRank += static_cast<int>(rank) + 1;
                if(rank == 0){
                    ++report.firstChoice;
                }
                break;
            }
        }
    }

    if(report.matched > 0){
        report.averageRank = static_cast<double>(totalAssignedRank) / report.matched;
    }
    if(report.totalStudents > 0){
        report.firstChoicePercent = 100.0 * report.firstChoice / report.totalStudents;
    }
    report.blockingPairs = countBlockingPairs(students, electives, assignedElectiveIds);
    return report;
}

int MatchingEngine::detectBlockingPairs() const {
    return countBlockingPairs(students, electives, getAssignedElectiveIds());
}

FairnessReport MatchingEngine::generateFairnessReport() const {
    return buildFairnessReport(students, electives, getAssignedElectiveIds());
}

const std::vector<Student>& MatchingEngine::getStudents() const {
    return students;
}

const std::vector<Elective>& MatchingEngine::getElectives() const{
    return electives;
}