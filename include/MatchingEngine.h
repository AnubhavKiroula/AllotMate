#pragma once

using namespace std;

class MatchingEngine {
    public:
        MatchingEngine() = default;
        ~MatchingEngine() = default;

        void runGaleShapley();
        void detectBlockingPairs();
        void generateFairnessReport();
};