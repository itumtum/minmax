#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <fstream>
#include <bitset>
#include <algorithm>
#include <string>

using namespace std;

ofstream dump("combinaties.txt", ios::out);

bool ONMOGELIJK = false;       //wordt true indien geen oplossing gevonden kan worden.
bool TOONDETAILS = false;      //zet op true om meer details te zien over elke combinatie die berekend is (cout).
bool TOONINPUT = true;         //zet op true om meer details te zien over input die gegeven werd en de combinaties die berekend gaan worden (cout).
bool TOONBEREKENING = true;   //zet op true om meer details te zien over de minima berekening (cout).
bool TOONELIMINATIE = true;    //zet op true om meer details te zien over reduceren van de resulterende spreidingen (cout).

size_t maxAantalUurlijsten = 64; //technische beperking omdat ik ergens een uint64_t gebruik om de uurlijstcombinaties voor te stellen.
size_t maxCombinaties = 10000;   //Beperken van het aantal combinaties van uurlijsten die we gaan berekenen
size_t maxOutputFactor = 100;    //Beperken van het aantal max spreidingen die we willen overhouden (factor x aantal input uurlijsten)

//Inputdata. Is wat binnenkomt.
//Er kan redundantie in de input zitten. Wordt eerst weggehaald. 
int AANTAL;                    //is de grootte van de uurlijsten waarmee we werken. Klassiek de lengte van het volledige rooster.
vector<vector<bool>> MAT;      //zijn de uurlijsten, als bool vector, (m stuks) waarop de minima gevraagd worden.
vector<int> MIN;               //zijn de minima, als int vector (m stuks). Per uurlijst het minimum dus.
int MAX;                       //is het aantal uur dat geplaatst gaat worden voor de resource waarvoor de spreidingen bepaald worden.

//---------------------------------------------------------------------------------------------------------------------------------------------------
/* Hoe werkt het?

De basis is dat we voor elke opgegeven uurlijst en minimum het omgekeerde vragen via een maximum spreiding.
Stel dat je op volgende uurlijst minimum 2 vraagt en je weet dat er in totaal 6 uur geplaatst worden:
- 0000001111 (min 2)
Dan kunnen we dat vertalen in volgende max spreiding
- 1111110000 (max 4) Waarbij 4 = 6 - 2.
Doe je dat voor elke uurlijst dan is het in theorie voor de BT voldoende om een oplossing te zoeken voor het minimumprobleem. Echter, er is meer uit te halen door verder te gaan dan de uurlijsten louter 1 per 1 te beschouwen.
Heb je 5 uurlijsten dan kan je ook de combinaties van 2 uit 5, van 3 uit 5, van 4 uit 5 en 5 uit 5 bekijken.

Neem je de combinatie van 2 uurlijsten dan krijg je bijvoorbeeld het volgende
- 0000001111 (min 2)
- 0011000000 (min 1)
Dan kunnen we dat vertalen in volgende max spreiding op een uurlijst die de inverse is van de unie van beide uurlijsten.
- 1100110000 (max 3) Waarbij 3 = 6 - (2 + 1)

Dit is juist omdat beide uurlijsten disjunct zijn. Stel dat ze dat niet zijn en dus een overlap hebben:
- 0000001111 (min 2)
- 0011001000 (min 1)
Dan kunnen we dat vertalen in volgende max spreiding op een uurlijst die de inverse is van de unie van beide uurlijsten.
- 1100110000 (max 4) Waarbij 4 = 6 - (2 + 1) + 1. Die laatste + 1 is hier de overlap. Dus, door 2 uur te plaatsten kunnen we aan beide minima voldoen. Dwz dat op de inverse max 4 mag staan.

Het probleem wordt dus complexer als er overlap tussen opgegeven uurlijsten ontstaat.
Vanaf dan is het niet meer eenvoudig te bepalen omdat je meerdere soorten overlaps gaat krijgen tussen verschillende uurlijsten in de combinatie.
Als we niet naar een algo willen grijpen dat alle mogelijkheden uitprobeert (kan heel lang duren), dan moeten we een manier vinden om het met minder te doen maar het moet wel correct zijn.
- We gebruiken een algo dat via een vaste strategie het minimum bepaalt
- We moeten dan zeker zijn dat we enkel die combinaties aanbieden waarvoor dat algo ook tot de beste oplossing kan komen,.
Het eenvoudige algo dat we gebruiken werkt als volgt:
- Je hebt uurlijsten/regionlijsten en je hebt de minima waaraan je moet voldoen.
- Voor elke uur/region heb je de capaciteit van wat er aan uren gezet kan worden. (als het een uurlijst is is dat steeds <= 1. Met regions, die uren groeperen, is de capaciteit de som van de capaciteiten van de uren)
- We vertrekken van 0 uur geplaatst. In het algo houden we minimaLeft en capacityLeft bij. minimaLeft zegt per uurlijst wat we nog resten te doen, capacityLeft zegt wat er nog over is per uur/region.
- We bekijken welke minimaLeft nog > 0 zijn. Dan zoeken we het uur/region dat zoveel mogelijk van deze resterende minima kan verbeteren, die kiezen we dan.
- We passen minimaLeft/capacityLeft aan en doen de oefening opnieuw tot alle minima voldaan zijn.
- algo zit in "bepaalMinimalePlaatsingSimpel()"
Dit algo werkt goed op voorwaarde dat de combinatie van uurlijsten aan bepaalde voorwaarden voldoet.
- Die voorwaarde wordt bekeken in "zitErEenSlechtLoopInDeMatrix()"
- Deze stelt een soort grafe op van de info in de matrix (rijen zijn uur/regionlijsten, kolommen zijn uren/regions)
- Dan kijken we of je vertrekkende van een uur/regionlijst Lx via een uniek pad over regions ry terug bij Lx kunt komen (lengte > 1)
-- stel dat je bijvoorbeeld volgend pad kan afleggen vertrekkende van L1: ((-,L1),(r7,L3),(r6,L4),(r2,L2),(r9,L1)) --> Loop in de matrix
-- je gaat wel steeds verder vie unieke regions, een region gebruik je dus maar 1 keer in je pad.
-- als je dan terug kan komen tot een reeds bezochte Lx (of je startpunt) en de lengte van de loop is > 1 dan heb je een probleem en kan het algo dat niet aan.
-- dergelijke combinaties gaan we dan verwerpen.

Voorbeeld:
   1234567 (regions/uren)
L1 1001101
L2 0101011
L3 0010111

- vertrekkende van L1 kunnen we bijvoorbeeld volgend pad afleggen:
-- (-,L1),(r7,L3),(r6,L2),(r4,L1): slechte loop.

Deze combinatie wordt dan verworpen en niet berekend.
Het zoeken van deze loops is ook recursief maar qua complixiteit valt dat zeer goed mee, duurt niet lang, ook niet voor grote combinaties van uurlijsten.

Van zodra een combinatie op deze manier verworpen wordt voeg ik ze toe aan aan blackList. Elke andere, grotere combinatie die een blackListed combinatie als subset bevat kan dan ook snel verworpen worden.
Dus via de blackList kunnen we sneller tot een goede selectie aan combinaties komen.

Dus, voor elke combinatie van input uurlijsten zouden we een extra helpende max-spreiding kunnen afleiden.
Voor hoeveel combinaties moeten we het proberen?
- In theorie kan het voor elke combinatie. heb je m uurlijsten dan kan je 2^m - 1 combinaties bekijken.
- omdat dit hoog kan oplopen wordt het aantal combinaties beperkt door de parameter maxCombinaties.
Als de combinaties beperkt moeten worden is de vraag hoe we ze gaan selecteren. De keuze die hier gemaakt is, is als volgt:
- via "voegCombinatiesVanKleinNaarGrootToe()"
- combinaties 1 uit m (zijn er m)
- combinaties 2 uit m
- combinaties 3 uit m
- ...
- we gaan dus van klein naar groot met een maximum van maxCombinaties

Voor elke combinatie die we berekenen gaan we vervolgens de unie van de uurlijsten bepalen, daar het inverse van nemen, en daar een max spreiding opzetten die bepaald wordt door MAX - "gevonden minimum combinatie".
Na deze berekening volgt nog een reductie van het aantal combinaties. Een uurlijst/max kan bijvoorbeeld geïmpliceerd worden door een andere uurlijst/max. Dan heeft het geen zin om de eerste te behouden.
Het kan ook zijn dat de max-waarde gelijk is aan het aantal uren in de uurlijst. Dan is hij ook overbodig.
Een grote reductie zien we dan vooral wanneer er veel overlap zit in de input uurlijsten.
Na deze reductiestap kan het zijn dat er nog overdreven veel spreidingen overblijven. We moeten hier ook niet overdrijven dus gaan we via de parameter maxOutputFactor het aantal verder beperken.
- deze maxOutputFactor * aantal (opgekuiste) input uurlijsten bepaalt hoeveel spreidingen we overhouden.
- in de volgorde dat ze toegevoegd werden worden de combinaties ook behouden tot het max aantal bereikt is.
- de spreidingen die zeker moeten opgenomen worden zijn de essentiele en dan zijn de combinaties uit de groep (1 uit m).
- "elimineerRedundanteCombinaties()" doet deze opkuis.

Merk op dat min/max spreidingen op volgende manier elkaar kunnen impliceren, de strengste eis blijft overeind (A en B zijn uurlijsten):
- Voor een min spreiding:
-- indien (A subset van B) en (min(A) >= min(B)): A impliceert B en B-min(B) kan dus vervallen.
-- wordt toegepast om de input spreidingen (min) te reduceren.

- Voor een max spreiding:
-- indien (A subset van B) en (max(A) >= max(B)): B impliceert A en A-max(A) kan dus vervallen.
-- wordt toegepast om de output spreidingen (max) te reduceren.

Het process:
- bekijk input uurlijsten en minima en reduceer indien mogelijk.
- bepaal of het geheel iets is met niet berekenbare combinaties.
- bepaal de combinaties die bekeken kunnen worden
- bereken het minimum voor elke combinatie
- voor alle berekende combinaties de redundante er uit halen en als er teveel zijn nog verder snoeien.
- wordt nu als test in "spreidingen.txt" geschreven. Die infor kan in een klassieke rooster.xls geplakt worden om te importeren in Mondriaan:
-- tabblad Week voor de uurlijsten 
-- tabblad Spreiding voor de max spreidingen
-- tabblad Opdracht voor 1 opdracht voor resource PETER voor het "aantal" uren dat in totaal geplaatst moet worden.
-- ook moet je de juiste weekstructuur nog opzetten in de excel alvorens de import te doen.

Enkele datastructuren:

struct Combinatie:
- bevat de nodige info om de berekening voor een combinatie te doen alsook het resultaat ervan.
- voor de berekening wordt er een CapacityMatrix afgeleid waarin de echte berekening gebeurt. Dit object wordt nadien weer verwijderd en de resultaten ervan bijgehouden.
- er wordt naast een lijst van alle bekeken combinaties ook een blackList van combinaties bijgehouden.

struct CapacityMatrix:
- voor elke combinatie wordt er eentje aangemaakt. Deze bevat de kennis om de berekening te doen.
- een capacitymatrix werkt eerder met regions dan met uren om de berekening te vereenvoudigen, te versnellen.
- voor elke combinatie waarvoor er een gemaakt wordt; worden telkens obv de opgegeven uurlijsten in de combo de regions opnieuw bepaald.
- voorbeeld van de omzetting van uurlijsten naar regions. Stel dat we volgende 2 uurlijsten hebben. Samen hebben ze een capaciteit om 6 uur te plaatsen.
-- 1234567890
-- 0000001111 (min 2) = R3 + R4
-- 0011001000 (min 1) = R2 + R3
-- **--**---- (cap 0): R1, lege region, niet gebruikt
-- --**------ (cap 2): R2, op deze kunnen 2 roosterpunten staan
-- ------*--- (cap 1): R3, op deze kan 1 roosterpunten staan
-- -------*** (cap 3): R4, op deze kunnen 3 roosterpunten staan
- Het minimumprobleem oplossen voor de combinatie van die 2 uurlijsten
- dus het probleem oploossen voor
-- 0000001111 (min 2)
-- 0011001000 (min 1)
--   11  1111 (cap 6)
- komt dus neer op het oplossen van
-- 234 (regions)
-- 011 (uurlijst 1 als regioncombi) (min2)
-- 110 (uurlijst 2 als regioncombi) (min1)
-- 213 (capaciteit van elke region, is ook samen 6)
- dus, de regionvoorstelling is een vereenvoudiging en als die opgelost wordt (minimum gevonden) is dat ook zo voor de uurvoorstelling

- voor elke combinatie wordt een capacity matrix gemaakt via een reduceerCapacityMatrix(combo) uitgevoerd op de static Combinatier::regionMatrix.
- de resulterende cm wordt dan gebruikt om de berekening te doen.
- de functie bepaalMinimalePlaasting() gaat de bezetting berekenen.

Voorbeelden:
- achteraan in main() staan een 10 tal voorbeelden. Zet de juiste uit commentaar om te proberen.

*/
//---------------------------------------------------------------------------------------------------------------------------------------------------
//Wat kleine hulpfuncties

int aantalTrue(const vector<bool>& vec) {
    int grootte = 0;
    for (const bool& v : vec) {
        if (v) {
            grootte++;
        }
    }
    return grootte;
}

bool isAEenSubSetVanB(const vector<bool>& A, const vector<bool>& B) {
    bool subset = true;
    if (A == B) {
        return true;
    }
    if ((A.size() == B.size()) && (aantalTrue(A) < aantalTrue(B))) {
        for (size_t n = 0; n < A.size() && subset; n++) {
            if (A[n]) {
                subset = B[n];
            }
        }
    }
    else {
        return false;
    }
    return subset;
}

bool isAEenSubSetVanB(const vector<bool>& A, int aantalA, const vector<bool>& B, int aantalB) {
    bool subset = true;
    if (A == B) {
        return true;
    }
    if ((A.size() == B.size()) && (aantalA < aantalB)) {
        for (size_t n = 0; n < A.size() && subset; n++) {
            if (A[n]) {
                subset = B[n];
            }
        }
    }
    else {
        return false;
    }
    return subset;
}


static size_t berekenCombinatie(double grootte, double totaal) {
    double result = 1;
    double tellerVan, tellerTot;
    double noemer;
    tellerVan = totaal;
    if ((totaal - grootte) > grootte) {
        tellerTot = (totaal - grootte) + 1;
        noemer = grootte;
    }
    else {
        tellerTot = grootte + 1;
        noemer = (totaal - grootte);
    }
    for (double i = 0; i <= (tellerVan - tellerTot); i++) {
        result = result * (tellerVan - i);
        if (i < noemer) {
            result = result / (noemer - i);
        }
    }
    return (size_t)result;
}

void coutBoolVector(vector<bool>& vec) {
    for (bool b : vec) {
        cout << (b ? "*" : "-");
    }
}

void printBoolVector(const vector<bool>& vec, ofstream& f) {
    for (size_t n = 0; n < vec.size(); n++) {
        if (vec[n]) {
            f << n + 1 << " ";
        }
    }
}

void printBoolVectorStars(vector<bool>& vec, ofstream& f) {
    for (bool b : vec) {
        f << (b ? "*" : "-");
    }
}

//---------------------------------------------------------------------------------------------------------------------------------------------------

struct CapacityMatrix {
    vector<vector<bool>> matrix;        //de uur/region lijsten
    vector<int> capacity;               //capaciteit van de uren/regions
    vector<int> minima;                 //te bereiken minima voor elke lijst
    vector<int> capacityLeft;           //capaciteit van de uren/regions die tijdens de berekening vermindert
    vector<int> minimaLeft;             //te bereiken minima voor elke lijst die tijdens de berekening vermindert
    bool slechteLoopInDeMatrix;         //indicatie dat het minimum niet berekend kan worden zonder alles uit te proberen. vervalt.
    int bezetting;
    vector<vector<bool>> regionMapping; //na reductie kan het nuttig zijn om eens te kijken hoe de nieuwe regions op de input regions/uren mappen.

    CapacityMatrix() : slechteLoopInDeMatrix(false), bezetting(0) {}
    CapacityMatrix(const vector<vector<bool>>& mat, const vector<int>& cap, const vector<int>& min) : bezetting(0) {
        matrix = mat;
        capacity = cap;
        minima = min;
        zitErEenSlechtLoopInDeMatrix();
    }

    CapacityMatrix(const vector<vector<bool>>& mat, const vector<int>& min) : bezetting(0) {
        //default capacity = alles 1
        matrix = mat;
        capacity.assign(mat[0].size(), 1);
        minima = min;
        zitErEenSlechtLoopInDeMatrix();
    }

    ~CapacityMatrix() {}

    bool zitErEenSlechtLoopInDeMatrixRecursive(vector<bool>& usedRegionList, vector<bool>& usedRegion, vector<size_t>& pathRegionList, size_t pathLength) {
        size_t m = matrix.size();
        size_t n = matrix[0].size();
        size_t startPoint = pathRegionList[pathLength];
        size_t targetPoint;
        //Probeer eerst via een afstand > 1 een reeds bezochte regionList te bereiken via een niet gebruikte region. Dan hebben we een slechte loop te pakken.
        bool closeTheLoop = false;
        for (size_t onPath = 0; onPath + 1 < pathLength && !closeTheLoop; onPath++) {
            targetPoint = pathRegionList[onPath];
            //bestaat er tussen die 2 een niet gebruikte region, dan hebben we een loop
            for (size_t nx = 0; nx < n && !closeTheLoop; nx++) {
                closeTheLoop = (matrix[startPoint][nx] && matrix[targetPoint][nx] && !usedRegion[nx]);
            }
        }
        if (closeTheLoop) {
            return true;
        }
        //als we niet verder kunnen op dit path geven we false terug. We kunnen niet verder als we alle regionList bezocht hebben of indien we van het startpunt geen andere nieuwe kunnen bereiken via een niet gebruikte region.
        if (pathLength + 1 == m) {
            return false;
        }
        bool pathCanContinue = false;
        for (size_t mx = 0; mx < m && !pathCanContinue; mx++) {
            if (!usedRegionList[mx]) {
                for (size_t nx = 0; nx < n && !pathCanContinue; nx++) {
                    pathCanContinue = (matrix[startPoint][nx] && matrix[mx][nx] && !usedRegion[nx]);
                }
            }
        }
        if (!pathCanContinue) {
            return false;
        }
        //hier gaan we verdere paden exploreren
        for (size_t mx = 0; mx < m && !closeTheLoop; mx++) {
            if (!usedRegionList[mx]) {
                for (size_t nx = 0; nx < n && !closeTheLoop; nx++) {
                    if (matrix[startPoint][nx] && matrix[mx][nx] && !usedRegion[nx]) {
                        usedRegionList[mx] = true;
                        usedRegion[nx] = true;
                        pathRegionList[pathLength + 1] = mx;
                        closeTheLoop = zitErEenSlechtLoopInDeMatrixRecursive(usedRegionList, usedRegion, pathRegionList, pathLength + 1);
                        usedRegionList[mx] = false;
                        usedRegion[nx] = false;
                    }
                }
            }
        }
        return closeTheLoop;
    }

    void zitErEenSlechtLoopInDeMatrix() {
        vector<bool> usedRegionList;
        vector<bool> usedRegion;
        vector<size_t> pathRegionList;
        pathRegionList.assign(matrix.size(), -1);
        bool loopAanwezig = false;
        for (size_t mx = 0; mx < matrix.size() && !loopAanwezig; mx++) {
            usedRegionList.assign(matrix.size(), false);
            usedRegion.assign(matrix[0].size(), false);
            usedRegionList[mx] = true; //startpunt van de loop.
            pathRegionList[0] = mx;
            loopAanwezig = zitErEenSlechtLoopInDeMatrixRecursive(usedRegionList, usedRegion, pathRegionList, 0);
        }
        slechteLoopInDeMatrix = loopAanwezig;
    }

    CapacityMatrix* reduceerCapacityMatrix(const vector<bool>& combo, bool mapRegionToRegion = false) const {
        //this: (m x n) matrix: m uurlijsten, n uren/regions + n capacity voor uren/regions
        //outMatrix: (p x q) matrix: p uurlijsten, q uren/regions + q capacity voor uren/regions
        //combo (m): m combinatie van te projecteren uurlijsten (p staan op true)
        CapacityMatrix* outMatrix;
        vector<vector<bool>> transposeMat;
        vector<int> ids;
        vector<bool> idsKeep;
        size_t m, n, p, q, mx, nx, px, qx;
        m = matrix.size();
        n = capacity.size();
        p = aantalTrue(combo);
        transposeMat.assign(n, {});
        for (size_t nx = 0; nx < n; nx++) {
            transposeMat[nx].assign(p, false);
            px = 0;
            for (mx = 0; mx < m; mx++) {
                if (combo[mx]) {
                    transposeMat[nx][px++] = matrix[mx][nx];
                }
            }
        }
        q = 0;
        ids.assign(n, -1);
        idsKeep.assign(n, false);
        for (size_t n1 = 0; n1 < n; n1++) {
            if (ids[n1] == -1) {
                ids[n1] = q++;
                idsKeep[n1] = true;
                for (size_t n2 = n1 + 1; n2 < n; n2++) {
                    if (transposeMat[n1] == transposeMat[n2]) {
                        ids[n2] = ids[n1];
                    }
                }
            }
        }
        //construeer output matrix. q is het nieuwe aantal regions.
        outMatrix = new(CapacityMatrix);
        outMatrix->capacity.assign(q, 0);
        for (nx = 0; nx < n; nx++) {
            outMatrix->capacity[ids[nx]] += capacity[nx];
        }
        outMatrix->matrix.assign(p, {});
        for (px = 0; px < p; px++) {
            outMatrix->matrix[px].assign(q, false);
            qx = 0;
            for (nx = 0; nx < n; nx++) {
                if (idsKeep[nx]) {
                    outMatrix->matrix[px][qx++] = transposeMat[nx][px];
                }
            }
        }
        outMatrix->minima.assign(p, 0);
        px = 0;
        for (mx = 0; mx < m; mx++) {
            if (combo[mx]) {
                outMatrix->minima[px++] = minima[mx];
            }
        }
        outMatrix->zitErEenSlechtLoopInDeMatrix();
        //vul regionMapping matrix indien nodig (enkel ter illustratie)
        if (mapRegionToRegion) {
            outMatrix->regionMapping.assign(q, {});
            for (qx = 0; qx < q; qx++) {
                outMatrix->regionMapping[qx].assign(n, false);
                for (nx = 0; nx < n; nx++) {
                    outMatrix->regionMapping[qx][nx] = (ids[nx] == qx);
                }
            }
        }
        return outMatrix;
    }

    void toonRegionMapping() {
        if (regionMapping.size() > 0) {
            cout << endl << "Aantal region uurlijsten: " << regionMapping.size();
            for (size_t i = 0; i < regionMapping.size(); i++) {
                cout << endl << i + 1 << "\t" ;
                coutBoolVector(regionMapping[i]);
                cout << " (CAP:" << aantalTrue(regionMapping[i]) << ")";
            }
        }
    }

    int verminderMinimaMetEen() {
        int returnVal = 0; //0: Ok maar nog niet opgelost. 1: opgelost (alle minima weggewerkt). -1: geen oplossing mogelijk
        int maxHits = 0;
        int maxN = -1;
        bool allesOpgelost = false;
        vector<int> hits;
        hits.assign(capacityLeft.size(), 0);
        for (size_t n = 0; n < capacityLeft.size(); n++) {
            if (capacityLeft[n] > 0) {
                for (size_t m = 0; m < matrix.size(); m++) {
                    if (minimaLeft[m] > 0 && matrix[m][n]) {
                        hits[n]++;
                        if (hits[n] > maxHits) {
                            maxHits = hits[n];
                            maxN = n;
                        }
                    }
                }
            }
        }
        if (maxN != -1) {
            allesOpgelost = true;
            capacityLeft[maxN]--;
            for (size_t m = 0; m < minimaLeft.size(); m++) {
                if (matrix[m][maxN]) {
                    minimaLeft[m]--;
                }
                allesOpgelost = allesOpgelost && (minimaLeft[m] <= 0);
            }
            if (allesOpgelost) {
                returnVal = 1;
            }
        }
        else {
            returnVal = -1;
        }
        return returnVal;
    }

    int bepaalMinimalePlaatsingSimpel() {
        minimaLeft = minima;
        capacityLeft = capacity;
        int iteraties = 0;
        int result = 0;
        do {
            result = verminderMinimaMetEen();
            if (result != -1) {
                iteraties++;
            }
        } while (result == 0);
        return (result == 1) ? iteraties : -1;
    }

    void bepaalMinimalePlaatsing() {
        if (!slechteLoopInDeMatrix) {
            bezetting = bepaalMinimalePlaatsingSimpel();
        }
        else {
            bezetting = -1;
        }
    }
};

//---------------------------------------------------------------------------------------------------------------------------------------------------

struct Combinatie {
    static vector<vector<bool>> uurlijsten; //opgekuiste input uurlijsten
    static vector<int> minima;              //minima op de uurlijsten
    static int totaal;                      //totaal aantal te plaatsen uren voor de resource
    static vector<Combinatie*> combinaties; //lijst van combinaties die we bekijken
    static CapacityMatrix* uurMatrix;       //voorstelling uurlijsten/minima als CapacityMatrix
    static CapacityMatrix* regionMatrix;    //gereduceerde uurMatrix. Wordt het startpunt voor alle capacity matrices voor de combinaties.
    static vector<uint64_t> blackList;      //als een combinatie niet berekenbaar is wordt hij hier aan toegevoegd.

    vector<bool> combo;            //verwijzing naar de gecombineerde uurlijsten. Relatief tov input. Bij clusters tov combinatie.
    vector<bool> inverseUurlijst;  //inverse unie van de door combo gecombineerde uurlijsten
    int aantalUur;                 //aantal uren op true in inverseUurlijst (om verder te gebruiken vanwege performantie)
    int minimum;                   //het gevonden minimum voor deze combinatie (-1 indien geen oplossing gevonden en alles moet verworpen worden)
    int maximum;                   //is het totaal - minimum. Wordt het maximum voor de spreiding.
    int capaciteit;                //de capaciteit van inverseUurlijst. (aantal uren 1 in de lijst)
    bool essentieel;               //is het een elementaire combinatie (1 uit m). Moet steeds behouden blijven indien er teveel zouden zijn.
    bool berekenbaar;              //is de combinatie berekenbaar. indien niet, verwerpen.
    bool behouden;                 //combinatie weerhouden? Combinaties kunnen geïmpliceerd worden daar andere. Kunnen dus wegvallen zonder gevaar.
    uint64_t comboId;              //comboId maken om sneller bitwise operaties te kunnen doen.
 
    Combinatie() : minimum(-1), maximum(-1), capaciteit(-1), essentieel(false), behouden(true), berekenbaar(true), comboId(0), aantalUur(0) {}
    Combinatie(vector<bool>& newCombo) : combo(newCombo), minimum(-1), maximum(-1), capaciteit(-1), essentieel(false), behouden(true), berekenbaar(true), aantalUur(0) {
        bepaalinverseUurlijst();
        //indien in de combo slechts één uurlijst staat dan is hij essentieel. Is ter bescherming omdat we niet weten welke combo's overleven door de maxOutputFactor.
        essentieel = (aantalTrue(combo) == 1);
        //comboId maken om sneller bitwise operaties te kunnen doen.
        bitset<64> comboIdBitset;
        for (size_t i = 0; i < combo.size(); i++) {
            comboIdBitset[i] = combo[i];
        }
        comboId = comboIdBitset.to_ullong();
    }

    static void kuisInputUurlijstenOp(const vector<vector<bool>>& inputUurlijsten, const vector<int>& inputMinima) {
        vector<bool> keep;
        keep.assign(inputUurlijsten.size(), true);
        for (size_t mx1 = 0; mx1 < inputUurlijsten.size(); mx1++) {
            for (size_t mx2 = mx1 + 1; mx2 < inputUurlijsten.size(); mx2++) {
                //als A een subset is van B (of A == B) en minA >= minB : B vervalt
                if (isAEenSubSetVanB(inputUurlijsten[mx1], inputUurlijsten[mx2])) {
                    if (inputMinima[mx1] >= inputMinima[mx2]) {
                        keep[mx2] = false;
                    }
                }
                //omgekeerd
                else if (isAEenSubSetVanB(inputUurlijsten[mx2], inputUurlijsten[mx1])) {
                    if (inputMinima[mx2] >= inputMinima[mx1]) {
                        keep[mx1] = false;
                    }
                }
            }
        }
        size_t newSize = aantalTrue(keep);
        size_t ix = 0;
        uurlijsten.assign(newSize, {});
        minima.assign(newSize, 0);
        for (size_t mx = 0; mx < keep.size(); mx++) {
            if (keep[mx]) {
                uurlijsten[ix] = inputUurlijsten[mx];
                minima[ix] = inputMinima[mx];
                ix++;
            }
        }
    }

    static void setupCombinaties(const vector<vector<bool>>& inputUurlijsten, const vector<int>& inputMinima, const int inputTotaal) {
        _ASSERT(inputMinima.size() == inputUurlijsten.size());
        _ASSERT(inputTotaal > 0);
        kuisInputUurlijstenOp(inputUurlijsten, inputMinima);
        _ASSERT(uurlijsten.size() <= maxAantalUurlijsten);
        Combinatie::totaal = inputTotaal;
        Combinatie::uurMatrix = new CapacityMatrix(Combinatie::uurlijsten, Combinatie::minima);
        vector<bool> combo;
        combo.assign(uurlijsten.size(), true);
        Combinatie::regionMatrix = uurMatrix->reduceerCapacityMatrix(combo, true);

        if (TOONINPUT) {
            cout << endl << "Aantal input uurlijsten: " << uurlijsten.size() << ". Totaal te plaatsen: " << totaal ;
            for (size_t i = 0; i < uurlijsten.size(); i++) {
                cout << endl << "MIN:" << minima[i] << "\t";
                coutBoolVector(uurlijsten[i]);
                cout << " (CAP:" << aantalTrue(uurlijsten[i]) << ")";
            }
            Combinatie::regionMatrix->toonRegionMapping();
        }
    }

    void bepaalinverseUurlijst() {
        static vector<vector<bool>>& matrix = Combinatie::uurlijsten;
        inverseUurlijst.assign(matrix[0].size(), false);
        for (size_t m = 0; m < matrix.size(); m++) {
            if (combo[m]) {
                for (size_t n = 0; n < inverseUurlijst.size(); n++) {
                    inverseUurlijst[n] = inverseUurlijst[n] || matrix[m][n];
                }
            }
        }
        inverseUurlijst.flip();
        aantalUur = aantalTrue(inverseUurlijst);
        capaciteit = 0;
        for (size_t c = 0; c < inverseUurlijst.size(); c++) {
            if (inverseUurlijst[c]) { capaciteit++; }
        }
    }

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

    static void voegCombinatiesVanKleinNaarGrootToe() {
        //best te gebruiken bij methode2 problemen omdat we dan minder grote combinaties hebben (rekentijd!)
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

    static void toonCombinaties() {
        cout << endl << "Toegevoegde combinaties:" << endl;
        for (size_t c = 0; c < combinaties.size(); c++) {
            coutBoolVector(combinaties[c]->combo);
            cout << endl;
        }
    }

    static bool blackListed(uint64_t combId) {
        bool listed = false;
        for (size_t i = 0; i < blackList.size() && !listed; i++) {
            listed = (blackList[i] == (combId & blackList[i]));
        }
        return listed;
    }

    //bij TOONBEREKENING:
    // '.' = berekenbaar, '*' = bevat slechte loop (naar blackList), 'x' = een subset zit in de blacklist dus ook verworpen.
    static void berekenMinima() {
        if (TOONINPUT) {
            cout << endl << "Berekenen:(" << combinaties.size() << ")" << endl;
        }
        CapacityMatrix* comboMatrix;
        for (Combinatie* c : combinaties) {
            //eerst kijken of hij een subset is van een blacklisted combinatie
            if (blackListed(c->comboId)) {
                c->berekenbaar = false;
                if (TOONBEREKENING) { cout << "x"; }
            }
            else {
                comboMatrix = Combinatie::regionMatrix->reduceerCapacityMatrix(c->combo);
                c->berekenbaar = !comboMatrix->slechteLoopInDeMatrix;
                if (c->berekenbaar) {
                    if (TOONBEREKENING) { cout << "."; }
                    comboMatrix->bepaalMinimalePlaatsing();
                    c->minimum = comboMatrix->bezetting;
                    c->maximum = (c->minimum > 0) ? (Combinatie::totaal - c->minimum) : -1;
                    if (c->maximum < 0) {
                        ONMOGELIJK = true;
                    }
                }
                else {
                    if (TOONBEREKENING) { cout << "*"; }
                    blackList.push_back(c->comboId);
                }
                delete comboMatrix;
            }
        }
        if (TOONBEREKENING) { cout << endl; }
    }

    static void dumpCombinaties() {
        dump << "Combinatie | Inverse uurlijst | maximum | berekenbaar | weerhouden" << std::endl;
        for (size_t c = 0; c < combinaties.size(); c++) {
            printBoolVectorStars(combinaties[c]->combo, dump);
            dump << '\t';
            printBoolVectorStars(combinaties[c]->inverseUurlijst, dump);
            dump << '\t' << combinaties[c]->maximum << '\t' << combinaties[c]->berekenbaar << '\t' << combinaties[c]->behouden;
            dump << std::endl;
        }
    }

    static int berekenAantalNietBerekendebareCombinaties() {
        int tel = 0;
        for (const Combinatie* c : combinaties) {
            if (!c->berekenbaar) {
                tel++;
            }
        }
        return tel;
    }

    static int berekenAantalWeerhoudenCombinaties() {
        int tel = 0;
        for (const Combinatie* c : combinaties) {
            if (c->behouden) {
                tel++;
            }
        }
        return tel;
    }

    static void elimineerRedundanteCombinaties() {
        if (ONMOGELIJK) { return; }
        //1.eerst spreidingen verwijderen waarvoor de max waarde >= de capaciteit van de inverse uurlijst.
        if (TOONELIMINATIE) {
            cout << endl << "optimaliseer aantal spreidingen" << endl;
        }
        for (size_t c = 0; c < combinaties.size(); c++) {
            if (TOONELIMINATIE && div(c,1000).rem == 0) { cout << "1"; }
            combinaties[c]->behouden = ((combinaties[c]->berekenbaar) && (combinaties[c]->maximum < combinaties[c]->capaciteit));
        }
        if (TOONELIMINATIE) {
            cout << endl << "Aantal beperken door te kijken of het gestelde max niet >= capaciteit; of niet 0 is";
            cout << endl << "BEHOUDEN/VERVALLEN/TOTAAL: " << berekenAantalWeerhoudenCombinaties() << "/" << berekenAantalNietBerekendebareCombinaties() << "/" << combinaties.size() << endl;
        }
        //2.indien uurlijst A een subset is van B (of A == B) en max(A) >= max(B) dan vervalt A.
        for (size_t c1 = 0; c1 < combinaties.size(); c1++) {
            if (TOONELIMINATIE && div(c1, 10).rem == 0) { cout << "2"; }
            for (size_t c2 = c1 + 1; c2 < combinaties.size(); c2++) {
                if (combinaties[c1]->behouden && isAEenSubSetVanB(combinaties[c1]->inverseUurlijst, combinaties[c1]->aantalUur, combinaties[c2]->inverseUurlijst, combinaties[c1]->aantalUur)) {
                    //c1 is subset van c2.
                    if (combinaties[c1]->maximum >= combinaties[c2]->maximum) {
                        combinaties[c1]->behouden = false;
                    }
                }
                else if (combinaties[c2]->behouden && isAEenSubSetVanB(combinaties[c2]->inverseUurlijst, combinaties[c2]->aantalUur, combinaties[c1]->inverseUurlijst, combinaties[c1]->aantalUur)) {
                    //c2 is subset van c1.
                    if (combinaties[c2]->maximum >= combinaties[c1]->maximum) {
                        combinaties[c2]->behouden = false;
                    }
                }
            }
        }
        if (TOONELIMINATIE) {
            cout << endl << "Aantal beperken door te kijken of 2 uurlijsten elkaars subset zijn.";
            cout << endl << "BEHOUDEN/VERVALLEN/TOTAAL: " << berekenAantalWeerhoudenCombinaties() << "/" << berekenAantalNietBerekendebareCombinaties() << "/" << combinaties.size() << endl;
        }
        //3.we gaan het aantal behouden combinaties nu verder beperken door de maxOutputFactor.
        size_t tel = 0;
        size_t maxTeller = maxOutputFactor * uurlijsten.size();
        for (size_t c = 0; c < combinaties.size(); c++) {
            if (tel >= maxTeller && !combinaties[c]->essentieel) {
                combinaties[c]->behouden = false;
            }
            if (combinaties[c]->behouden) {
                tel++;
            }
        }
        if (TOONELIMINATIE) {
            cout << endl << "Aantal verder beperken door maxOutputFactor * aantal uurlijsten: " << maxOutputFactor << " * " << uurlijsten.size() << " = " << maxTeller;
            cout << endl << "BEHOUDEN/VERVALLEN/TOTAAL: " << berekenAantalWeerhoudenCombinaties() << "/" << berekenAantalNietBerekendebareCombinaties() << "/" << combinaties.size() << endl;
        }
    }

    static void toonOutput() {
        int weerhoudenCombinaties = 0;
        for (size_t c = 0; c < combinaties.size() && !ONMOGELIJK; c++) {
            if (TOONDETAILS) {
                cout << endl << "-----------------------------------------------------------------------------------------" << endl;
            }
            if (TOONDETAILS || combinaties[c]->behouden) {
                cout << "MAX:" << combinaties[c]->maximum << "\t";
                coutBoolVector(combinaties[c]->inverseUurlijst);
                cout << " (CAP:" << combinaties[c]->capaciteit << "/" << combinaties[c]->capaciteit - combinaties[c]->maximum << ")" << " (" << combinaties[c]->behouden << ")\t" << endl;
                if (TOONDETAILS) {
                    for (size_t a = 0; a < combinaties[c]->combo.size(); a++) {
                        if (combinaties[c]->combo[a]) {
                            cout << endl << "MIN:" << minima[a] << "\t";
                            coutBoolVector(uurlijsten[a]);
                            cout << " (CAP:" << aantalTrue(uurlijsten[a]) << ")";
                        }
                    }
                    cout << endl << "(min bezetting: " << combinaties[c]->minimum << "/" << Combinatie::totaal << ")" << endl;
                }
            }
            if (combinaties[c]->behouden) {
                weerhoudenCombinaties++;
            }
        }
        cout << "-----------------------------------------------------------------------------------------" << endl;
        if (ONMOGELIJK) {
            cout << "ALLES VERVALT, MINIMUM SPREIDING NIET MOGELIJK !" << endl;
        }
        else {
            cout << "WEERHOUDEN SPREIDINGEN: " << weerhoudenCombinaties << " Van de " << combinaties.size() << " Gekozen combinaties" << endl;
        }
    }

    static void printOutput(string filename) {
        ofstream sprFile(filename, ios::out);
        sprFile << "LENGTE ROOSTER: " << Combinatie::uurlijsten[0].size() << endl;
        sprFile << "AANTAL PLAATSEN BEZET: " << Combinatie::totaal << endl << endl;
        for (size_t m = 0; m < uurlijsten.size(); m++) {
            sprFile << "MIN: " << Combinatie::minima[m] << "\t\t";
            printBoolVector(Combinatie::uurlijsten[m], sprFile);
            sprFile << endl;
        }
        sprFile << endl;

        if (ONMOGELIJK) {
            sprFile << "ALLES VERVALT, MINIMUM SPREIDING NIET MOGELIJK !" << endl;
        }
        else {
            sprFile << "------------------------------------------------------------------------" << endl;
            sprFile << "tabblad Opdracht:" << endl;
            sprFile << "x\tPETER\t\t\t" << Combinatie::totaal << "\t1" << endl;
            sprFile << endl;
            sprFile << "------------------------------------------------------------------------" << endl;
            sprFile << "tabblad Spreiding:" << endl;
            for (size_t c = 0; c < combinaties.size(); c++) {
                if (combinaties[c]->behouden) {
                    sprFile << "x\tPETER\t\t\t\t\t" << combinaties[c]->maximum << "\tUL" << c << "_" << "M" << combinaties[c]->maximum << endl;
                }
            }
            sprFile << endl;
            sprFile << "------------------------------------------------------------------------" << endl;
            sprFile << "tabblad Week:" << endl;
            for (size_t c = 0; c < combinaties.size(); c++) {
                if (combinaties[c]->behouden) {
                    sprFile << "UL" << c << "_" << "M" << combinaties[c]->maximum << "\t\t";
                    printBoolVector(combinaties[c]->inverseUurlijst, sprFile);
                    sprFile << endl;
                }
            }
        }
        sprFile << endl;
        sprFile.close();
    }
};

vector<Combinatie*> Combinatie::combinaties;
vector<vector<bool>> Combinatie::uurlijsten;
vector<int> Combinatie::minima;
int Combinatie::totaal = 0;
CapacityMatrix* Combinatie::uurMatrix = nullptr;
CapacityMatrix* Combinatie::regionMatrix = nullptr;
vector<uint64_t> Combinatie::blackList;

//---------------------------------------------------------------------------------------------------------------------------------------------------

void zoekOplossing() {
    Combinatie::setupCombinaties(MAT, MIN, MAX);
    Combinatie::voegCombinatiesVanKleinNaarGrootToe();
    //Combinatie::toonCombinaties();
    Combinatie::berekenMinima();
    Combinatie::elimineerRedundanteCombinaties();
    Combinatie::dumpCombinaties();
    Combinatie::toonOutput();
    Combinatie::printOutput("spreidingen.txt");
}

//---------------------------------------------------------------------------------------------------------------------------------------------------

int main() {

// Voorbeeld 1:
    MAX = 8;
    AANTAL = 5;
    MAT.assign(AANTAL, {});
    //         1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40
    MAT[0] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0 };
    MAT[1] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 };
    MAT[2] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0 };
    MAT[3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 };
    MAT[4] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 };
    MIN = { 4, 3, 5, 2, 3 };

// Voorbeeld 2:
    //MAX = 10; //kleiner gaat niet
    //AANTAL = 3;
    //MAT.assign(AANTAL, {});
    //MAT[0] = { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[1] = { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[2] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 };
    //MIN = { 2, 4, 4 };

// Voorbeeld 3:
    //MAX = 8; //kleiner gaat niet.
    //AANTAL = 3;
    //MAT.assign(AANTAL, {});
    //MAT[0] = { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[1] = { 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[2] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 };
    //MIN = { 2, 4, 4 };

// Voorbeeld 4:
    //MAX = 12; //min 10 nodig!
    //AANTAL = 10;
    //MAT.assign(AANTAL, {});
    //MAT[0] = { 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0 };
    //MAT[1] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 };
    //MAT[2] = { 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0 };
    //MAT[3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 };
    //MAT[4] = { 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 };
    //MAT[5] = { 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 };
    //MAT[6] = { 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[7] = { 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[8] = { 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0 };
    //MAT[9] = { 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0 };
    //MIN = { 4, 3, 5, 2, 3, 4, 2, 2, 4, 3 };

 // Voorbeeld 5: alles disjunct
    //MAX = 18; //min 15 nodig!
    //AANTAL = 10;
    //MAT.assign(AANTAL, {});
    //MAT[0] = { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[1] = { 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[2] = { 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[3] = { 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[4] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[5] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[6] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0 };
    //MAT[7] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0 };
    //MAT[8] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0 };
    //MAT[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 };
    //MIN = { 1, 2, 1, 2, 1, 2, 1, 2, 1, 2 };

 // Voorbeeld 6: alles disjunct, groter. Merk op, bij lagere MAX gaat het aantal weerhouden spreidingen sterk omhoog
    //MAX = 32; //min 30 nodig!
    //AANTAL = 20;
    //MAT.assign(AANTAL, {});
    ////         1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40
    //MAT[0] = { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[1] = { 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[2] = { 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[3] = { 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[4] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[5] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[6] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[7] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[8] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[11] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[14] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[15] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0 };
    //MAT[17] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0 };
    //MAT[18] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0 };
    //MAT[19] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 };
    //MIN = { 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2 };

// Voorbeeld 7: stress test, duurt erg lang! grote clusters
    //MAX = 24; //min xx nodig!
    //AANTAL = 10;
    //MAT.assign(AANTAL, {});
    ////         1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40
    //MAT[0] = { 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0 };
    //MAT[1] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 };
    //MAT[2] = { 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0 };
    //MAT[3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 };
    //MAT[4] = { 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 };
    //MAT[5] = { 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 };
    //MAT[6] = { 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[7] = { 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[8] = { 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0 };
    //MAT[9] = { 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0 };
    //MIN = { 8, 6, 10, 4, 6, 8, 4, 4, 8, 6 };

 //Voorbeeld 8: minder overlappingen, snel klaar
    //MAX = 34; //min 34 nodig!
    //AANTAL = 10;
    //MAT.assign(AANTAL, {});
    //MAT[0] = { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 };
    //MAT[1] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0 };
    //MAT[2] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 };
    //MAT[4] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
    //MAT[5] = { 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[6] = { 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[7] = { 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[8] = { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MIN = { 8, 6, 10, 4, 6, 5, 4, 4, 7, 6 };

 // Voorbeeld 9: meer combi, minder overlappingen, inputuurlijsten vervallen, snel klaar
    //MAX = 36; //min 35 nodig!
    //AANTAL = 32;
    //MAT.assign(AANTAL, {});
    ////          1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42
    //MAT[0]  = { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0 };
    //MAT[1]  = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0 };
    //MAT[2]  = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[3]  = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0 };
    //MAT[4]  = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[5]  = { 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[6]  = { 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[7]  = { 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[8]  = { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[9]  = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[10] = { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1 };
    //MAT[11] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1 };
    //MAT[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
    //MAT[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1 };
    //MAT[14] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1 };
    //MAT[15] = { 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
    //MAT[16] = { 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
    //MAT[17] = { 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
    //MAT[18] = { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
    //MAT[19] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
    //MAT[20] = { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 };
    //MAT[21] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1 };
    //MAT[22] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 };
    //MAT[23] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1 };
    //MAT[24] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1 };
    //MAT[25] = { 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 };
    //MAT[26] = { 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 };
    //MAT[27] = { 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 };
    //MAT[28] = { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 };
    //MAT[29] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 };
    //MAT[30] = { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 };
    //MAT[31] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1 };
    //MIN = { 8, 6, 10, 4, 6, 5, 4, 4, 7, 6, 9, 7, 11, 5, 7, 6, 5, 5, 8, 7, 7, 5, 9, 3, 5, 4, 3, 3, 6, 5, 7, 5 };

// Voorbeeld 10: test met 1 uurlijst
    //MAX = 8; //kleiner gaat niet.
    //AANTAL = 1;
    //MAT.assign(AANTAL, {});
    //MAT[0] = { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MIN = { 2 };

// Voorbeeld 11: stress test, duurt erg lang! grote clusters
    //MAX = 24; //min xx nodig!
    //AANTAL = 20;
    //MAT.assign(AANTAL, {});
    ////         1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41
    //MAT[0] = { 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[1] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[2] = { 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0 };
    //MAT[3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0 };
    //MAT[4] = { 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0 };
    //MAT[5] = { 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
    //MAT[6] = { 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[7] = { 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[8] = { 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0 };
    //MAT[9] = { 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0 };
    //MAT[10] = { 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1 };
    //MAT[11] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1 };
    //MAT[12] = { 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1 };
    //MAT[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 };
    //MAT[14] = { 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 };
    //MAT[15] = { 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1 };
    //MAT[16] = { 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
    //MAT[17] = { 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1 };
    //MAT[18] = { 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1 };
    //MAT[19] = { 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1 };
    //MIN = { 8, 6, 10, 4, 6, 8, 4, 4, 8, 6, 9, 7, 11, 5, 7, 9, 5, 5, 9, 7};

 // Voorbeeld 12: alles disjunct
    //MAX = 18; //min 15 nodig!
    //AANTAL = 10;
    //MAT.assign(AANTAL, {});
    //MAT[0] = { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[1] = { 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[2] = { 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[3] = { 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[4] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[5] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 };
    //MAT[6] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0 };
    //MAT[7] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0 };
    //MAT[8] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0 };
    //MAT[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 };
    //MIN = { 1, 2, 1, 2, 1, 2, 1, 2, 1, 2 };

    zoekOplossing();
}
