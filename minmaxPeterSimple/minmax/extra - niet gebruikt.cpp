    static void voegCombinatiesVanBepaaldeGrootteToe(int grootte, size_t start, vector<bool>& comb, bool flip) {
        if (grootte > 0) {
            for (size_t m = start; m < comb.size() && (Combinatie::combinaties.size() < maxCombinaties); m++) {
                comb[m] = true;
                Combinatie::voegCombinatiesVanBepaaldeGrootteToe(grootte - 1, m + 1, comb, flip);
                comb[m] = false;
            }
        }
        else {
            if (flip) {
                comb.flip(); //omkeren
                Combinatie::combinaties.push_back(new Combinatie(comb));
                comb.flip(); //terug goed zetten
            }
            else {
                Combinatie::combinaties.push_back(new Combinatie(comb));
            }
        }
    }

    static void voegCombinatiesVanGrootNaarKleinToe() {
        size_t aantal = Combinatie::uurlijsten.size();
        size_t helft = div(aantal, 2).quot;
        bool even = (div(aantal, 2).rem == 0);
        vector<bool> comb;
        comb.assign(aantal, false);
        for (size_t g = aantal; g >= helft + 1 && (Combinatie::combinaties.size() < maxCombinaties); g--) {
            Combinatie::voegCombinatiesVanBepaaldeGrootteToe(aantal - g, 0, comb, true);
            if (TOONINPUT) {
                cout << endl << "Combinaties toevoegen (" << (g) << "/" << aantal << ") ";
                cout << Combinatie::combinaties.size() << "/" << berekenCombinatie(aantal - g, aantal);
                cout << "/" << maxCombinaties << "/" << pow(2, aantal) - 1;
            }
        }
        for (size_t g = helft; g <= 2 && (Combinatie::combinaties.size() < maxCombinaties); g--) {
            Combinatie::voegCombinatiesVanBepaaldeGrootteToe(g, 0, comb, false);
            if (TOONINPUT) {
                cout << endl << "Combinaties toevoegen (" << g << "/" << aantal << ") ";
                cout << Combinatie::combinaties.size() << "/" << berekenCombinatie(g, aantal);
                cout << "/" << maxCombinaties << "/" << pow(2, aantal) - 1;
            }
        }
        Combinatie::voegCombinatiesVanBepaaldeGrootteToe(1, 0, comb, true);
    }

    static void voegCombinatiesVanKleinNaarGrootToe() {
        size_t aantal = Combinatie::uurlijsten.size();
        size_t helft = div(aantal, 2).quot;
        bool even = (div(aantal, 2).rem == 0);
        vector<bool> comb;
        comb.assign(aantal, false);
        for (size_t g = 1; g <= helft && (Combinatie::combinaties.size() < maxCombinaties); g++) {
            Combinatie::voegCombinatiesVanBepaaldeGrootteToe(g, 0, comb, false);
            if (TOONINPUT) {
                cout << endl << "Combinaties toevoegen (" << g << "/" << aantal << ") ";
                cout << Combinatie::combinaties.size() << "/" << berekenCombinatie(g, aantal);
                cout << "/" << maxCombinaties << "/" << pow(2, aantal) - 1;
            }
        }
        for (size_t g = helft + 1; g <= aantal && (Combinatie::combinaties.size() < maxCombinaties); g++) {
            Combinatie::voegCombinatiesVanBepaaldeGrootteToe(aantal - g, 0, comb, true);
            if (TOONINPUT) {
                cout << endl << "Combinaties toevoegen (" << (g) << "/" << aantal << ") ";
                cout << Combinatie::combinaties.size() << "/" << berekenCombinatie(aantal - g, aantal);
                cout << "/" << maxCombinaties << "/" << pow(2, aantal) - 1;
            }
        }
    }
