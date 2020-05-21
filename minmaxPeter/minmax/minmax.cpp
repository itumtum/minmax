#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <fstream>
#include <bitset>
#include <algorithm>
#include <string>

using namespace std;

ofstream dump("dumpfile.txt", ios::out);

bool ONMOGELIJK = false;       //wordt true indien geen oplossing gevonden kan worden.
bool TOONDETAILS = false;      //zet op true om meer details te zien over elke combinatie die berekend is (cout).
bool TOONINPUT = true;         //zet op true om meer details te zien over input die gegeven werd en de combinaties die berekend gaan worden (cout).
bool TOONBEREKENING = false;   //zet op true om meer details te zien over de minima berekening (cout).
bool TOONELIMINATIE = true;    //zet op true om meer details te zien over reduceren van de resulterende spreidingen (cout).

size_t maxRecursion = 10000;   //Beperken van het aantal iteraties in Methode2 van de minimaberekening
size_t maxCombinaties = 10000; //Beperken van het aantal combinaties van uurlijsten die we gaan berekenen
size_t maxOutputFactor = 100;  //Beperken van het aantal max spreidingen die we willen overhouden (factor x aantal input uurlijsten)

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

Het probleem wordt dus complexer als er overlap tussen opgegeven uurlijsten ontstaat en vooral vanaf dat eenzelfde uurlijst met meer dan 1 andere overlapt.
Vanaf dan is het niet meer eenvoudig te bepalen omdat je meerdere soorten ovelaps gaat krijgen tussen verschillende uurlijsten ion de combinatie.
Het bepalen van wat minimum nodig is aan aantal uren voor het voldoen van de minima voor een combinatie van uurlijsten is het lastigste probleem in deze code.
- Voor combinaties waarbij geen overlap is, of waarbij 1 uurlijst niet met meer dan 1 andere overlapt, kan het minimum op een eenvoudige manier afgeleid worden.
-- BepaalMinimalePlaatsingMethode1(): quasi lineair toename met het aantal te plaatsen minima
- Voor combinaties waarbij overlap bestaat van 1 uurlijst met meer dan 1 andere wordt het een lastig probleem dat we hier trachten te zoeken door alle mogelijkheden recursief af te lopen.
-- BepaalMinimalePlaatsingMethode2(): exponentiele toename met aantal de plaatsen minima
-- we laten de looptijd van deze beperken door de parameter maxRecursion. Gaan we er over dan stopt de routine en is de combinatie "niet berekenbaar" en valt ze weg.
-- het startpunt om te verbeteren voor deze methode is de outcome van methode 1. We laten die ook altijf eerst los op elke combinatie.

Dus, voor elke combinatie van input uurlijsten zouden we een extra helpende max-spreiding kunnen afleiden.
Voor hoeveel combinaties moeten we het proberen?
- In theorie kan het voor elke combinatie. heb je m uurlijsten dan kan je 2^m - 1 combinaties bekijken.
- omdat dit hoog kan oplopen wordt het aantal combinaties beperkt door de parameter maxCombinaties.
Als de combinaties beperkt moeten worden is de vraag hoe we ze gaan selecteren. De keuze die hier gemaakt is, is als volgt:
- indien er in de (opgekuiste) opgave (MAT/MIN) geen enkele uurlijst zit die met meer dan 1 andere overlapt dan worden de combinaties als volgt toegevoegd:
-- combinatie m uit m (is er 1, alles samen)
-- combinaties 1 uit m (zijn er m) en m-1 uit m (zijn er m)
-- combinaties 2 uit m en (m-2) uit m
-- combinaties 3 uit m en (m-3) uit m
-- ...
- dit gaat via voegCombinatiesSymmetrischToe()
- indien er in de (opgekuiste) opgave (MAT/MIN) wel uurlijsten zitten die met meer dan 1 andere overlapt dan worden de combinaties als volgt toegevoegd:
-- combinaties 1 uit m
-- combinaties 2 uit m
-- combinaties 3 uit m
-- ...
- dit gaat via voegCombinatiesVanKleinNaarGrootToe()
De reden voor dit verschil is dat het voor methode2 voor de minimumbepaling veel lastiger wordt met grote combinaties omdat het aantal te plaatsen minima dan sterk oploopt.
Het is een vrij arbitraire keuze maar het lijkt wel ok te zijn.

Voor elke combinatie die we berekenen gaan we vervolgens de unie van de uurlijsten bepalen, daar het inverse van nemen, en daar een max spreiding opzetten die bepaald wordt door MAX - "gevonden minimum combinatie".
Na deze berekening volgt nog een reductie van het aantal combinaties. Een uurlijst/max kan bijvoorbeeld geïmpliceerd worden door een andere uurlijst/max. Dan heeft het geen zin om de eerste te behouden.
Het kan ook zijn dat de max-waarde gelijk is aan het aantal uren in de uurlijst. Dan is hij ook overbodig.
Een grote reductie zien we dan vooral wanneer er veel overlap zit in de input uurlijsten.
Na deze reductiestap kan het zijn dat er nog overdreven veel spreidingen overblijven. We moeten hier ook niet overdrijven dus gaan we via de parameter maxOutputFactor het aantal verder beperken.
- deze maxOutputFactor * aantal (opgekuiste) input uurlijsten bepaalt hoeveel spreidingen we overhouden.
- in de volgorde dat ze toegevoegd werden worden de combinaties ook behouden tot het max aantal bereikt is.
- de spreidingen die zeker moeten opgenomen worden zijn de essentiele en dan zijn de combinaties uit de groep (1 uit m).
- elimineerRedundanteCombinaties() doet deze opkuis.

Merk op dat min/max spreidingen op volgende manier elkaar kunnen impliceren, de strengste eis blijft overeind (A en B zijn uurlijsten):
- Voor een min spreiding:
-- indien (A subset van B) en (min(A) >= min(B)): A impliceert B en B-min(B) kan dus vervallen.
-- wordt toegepast om de input spreidingen (min) te reduceren.

- Voor een max spreiding:
-- indien (A subset van B) en (max(A) >= max(B)): B impliceert A en A-max(A) kan dus vervallen.
-- wordt toegepast om de output spreidingen (max) te reduceren.

Het process:
- bekijk input uurlijsten en minima en reduceer indien mogelijk.
- bepaal of het geheel iets is met lastige combinaties (1 uurlijst met meer dan 1 andere in overlap).
- bepaal de combinaties die bekeken kunnen worden:
-- indien complex: van klein naar groot
-- indien niet complex: symmetrisch
- bereken het minimum voor elke combinatie
-- bekijken of de combinatie zelf eerst nog kan opgesplitst worden in afzonder oplosbare disjuncte clusters.
-- voor de gehele combinatie of voor zijn cluster constituenten bereken:
--- via methode1 voor allemaal
--- indien een complexe combinatie ook nog eens via methode 2
--- verwerp de hele combinatie als hij of een van zijn clusters niet berekend kan worden
- voor alle berekende combinaties de redundante er uit halen en als er teveel zijn nog verder snoeien.
- wordt nu als test in "spreidingen.txt" geschreven. Die infor kan in een klassieke rooster.xls geplakt worden om te importeren in Mondriaan:
-- tabblad Weken voor de uurlijsten 
-- tabblad Spreidingen voor de max spreidingen
-- wat je zelf moet aanvullen in de spreadsheet is de opdracht om "aantal" uren te plaatsten voor de resource PETER.
-- ook moet je de juiste weekstructuur nog opzetten in de excel alvorens de import te doen.

Enkele datastructuren:

- struct Bezetting:
-- voor een berekende combinatie of subcombinatie (via clusters) het berekende minimum. We houden zowel die van methode1 als methode2 hier in bij.
-- een map met alle tot nu toe berekende combinaties/subcombinaties: reedsBerekendeBezetting
--- omdat combinaties soms opgedeeld worden in clusters en clusters apart berekend worden (om performantieredenen voor methode 2), kan het zijn dat een combinatie meerdere keren voorkomt.
--- om die combinatie niet steeds opnieuw te berekenen worden die tussenresultaten hier bijgehouden.
--- via een unsigned 64bit int wordt een combinatie geïdentificeerd in deze map.

struct Combinatie:
- bevat de nodige info om de berekening voor een combinatie te doen alsook het resultaat ervan.
- voor de berekening wordt er een CapacityMatrix afgeleid waarin de echte berekening gebeurt. Dit object wordt nadien weer verwijderd en de resultaten ervan bijgehouden.

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

- Indien we methode2 moeten gebruiken dan kan het nuttig zijn om de gevraagde combinatie verder op te splitsen in clusters.
-- wordt uitgevoerd door verdeelInClusters(). Het probleem wordt indien mogelijk verder opgedeeld in deelproblemen die sneller uitgerekend kunnen worden.
-- de cm krijgt dan zelf een vector van kleinere capacity matrices die elk op hun beurt uitgerekend kunnen worden.
-- de som van hun minima kunnen dan gewoon opgeteld worden omdat ze onderling disjunct zijn.

- de functie bepaalMinimalePlaasting() gaat de bezetting berekenen. Ofwel voor zichzelf, ofwel als som van de uitkomst van haar clusters.
- als ze zelf moet berekend worden dan zal dat steeds eerst via methode1 gaan en indien er nood aan is, verder met methode 2.
- tijdens methode2 gaan we in recursie om alle mogelijke bezettingen af te lopen voor zover die niet over het minimum gaan dat methode1 gevonden heeft.
- De inputparameter voor methode2 is dus het minimum gevonden door methode1 - 1. methode 2 moet naar een waarde trachten te zoeken die kleiner is.
- omdat die recursieboom erg breed (aantal regions) en erg diep (te verbeteren minimum) kan worden, is het aangewezen zo snel mogelij in de boom te snoeien.
- dit kan door constant te monitoren of nog niet gehaalde minima op een of meerdere van de urlijsten nog gehaald kan worden binnen de gestelde limiet.
- de eenvoudigste manier zou zijn om inderdaad te monitoren hoeveel er nog op individuele uurlijsten geplaatst moet worden.
- een eerste uitbreiding hierop is het kijken naar de minima die nog gehaald moeten worden voor per 2 gecombineerde uurlijsten.
-- omdat we van 2 uurlijsten, overlap of niet, gemakkelijk kunnen bepalen hoeveel er nog minimum nodig is, kunnen we dat als betere maatstaf gebruiken.
-- dit gaat sneller tot snoeien leiden dan naar de enkelvoudige minima te kijken. Vergt wel wat extra administratie tijdens de recursie.
- een tweede verbetering hierop is bij een gecombineerd minimum (van 2 uurlijsten) de individuele minima bijtellen van andere disjuncte uurlijsten.
-- hiermee krijg je het zekere resterende nog hoger waardoor je nog vroeger kan afbreken.
- al deze technieken zitten in de struct Combi2MinimaCapacity

struct Combi2MinimaCapacity
- wordt voor methode2 als extra hulp object gebruikt in CapacityMatrix om beter te voorspellen of een recursie afgebroken mag worden.

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

void unionVectors(const vector<bool>& v1, const vector<bool>& v2, vector<bool>& vUnion) {
    _ASSERT(v1.size() == v2.size());
    for (size_t nx = 0; nx < v1.size(); nx++) {
        vUnion[nx] = v1[nx] || v2[nx];
    }
}

bool intersectionExists(const vector<bool>& v1, const vector<bool>& v2) {
    _ASSERT(v1.size() == v2.size());
    bool intersect = false;
    for (size_t nx = 0; nx < v1.size() && !intersect; nx++) {
        intersect = (v1[nx] && v2[nx]);
    }
    return intersect;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------

struct Bezetting {
    int gevondenBezetting1;           //gevonden via Methode1
    int gevondenBezetting2;           //gevonden via Methode2 obv resultaat Methode1
    bool berekenbaar;                 //indien bij methode2 het max aantal iteraties (maxRecursion) overschreden wordt dan wordt de combinatie niet weerhouden omdat we ze als niet berekenbaar aanduiden.

    Bezetting(int b1, int b2, bool b) : gevondenBezetting1(b1), gevondenBezetting2(b2), berekenbaar(b) {}

    static map<uint64_t, Bezetting*> reedsBerekendeBezetting;

    static void dumpReedsBerekendeBezetting() {
        dump << endl << "Reeds Berekende Bezetting: ";
        for (auto& x : reedsBerekendeBezetting) {
            if ((x.second->gevondenBezetting1 != x.second->gevondenBezetting2)) {
                dump << endl << x.first << " " << x.second->gevondenBezetting1 << " " << x.second->gevondenBezetting2 << " " << (x.second->gevondenBezetting1 == x.second->gevondenBezetting2);
            }
        }
        dump << endl;
    }
};

map<uint64_t, Bezetting*> Bezetting::reedsBerekendeBezetting;

//---------------------------------------------------------------------------------------------------------------------------------------------------

struct Combi2MinimaCapacity {
    //te gebruiken binnen CapacityMatrix voor Methode2. We gaan met de zekere mimima van elke combi van 2 al dan iet overlappende lijsten werken om methode2 performanter te maken.
    vector<int> combi2Capacity; //per 2 de (resterende) capaciteit van de doorsnede
    vector<int> combi2Minima; //per 2 het (resterende) minima
    vector<vector<size_t>> mappingUurlijst; //voor elke uurlijst (m) index de indices van de gecombineerde uurlijsten.
    vector<vector<size_t>> MappingUurlijstInvers; //voor elke index van de gecombineerde uurlijst de 2 indices van de orignele.
    vector<vector<size_t>> mappingUur; //voor elk uur/region (n) index de indexen van de gecombineerde uurlijsten (obv doorsnedes).
    int grootsteMinimum;
    bool grootsteMinimumDirty;

    vector<vector<bool>> unions; //tijdelijk
    vector<vector<size_t>> disjunctCombinations; //lijst van combi2 en minima die samengeteld zijn.
    vector<vector<size_t>> mappingUurlijstSumDisjunct; //voor elke uurlijst (m) index de indexen van de sommen.
    vector<int> combi2MinimaSum; //per 2 + disjuncte mogelijkheden het (resterende) minima

    Combi2MinimaCapacity() : grootsteMinimum(0), grootsteMinimumDirty(true) {}
    Combi2MinimaCapacity(const vector<vector<bool>>& matrix, const vector<int>& capacity, const vector<int>& minima) : grootsteMinimum(0), grootsteMinimumDirty(true) {
        size_t m = matrix.size(); //is ook min.size()
        size_t n = capacity.size(); //is ook mat[i].size()
        size_t nrIds = div(m * (m - 1), 2).quot;
        size_t idx = 0;
        //mappingUurlijst en inverse vullen. voor elke uurlijst index i alle indexen van gecombineerde uurlijsten waarin i participeert.
        //unions maken (voor koppels 4 bij 4)
        idx = 0;
        mappingUurlijst.assign(m, {});
        MappingUurlijstInvers.assign(nrIds, {});
        unions.assign(nrIds, {});
        for (size_t mx1 = 0; mx1 < m; mx1++) {
            for (size_t mx2 = mx1 + 1; mx2 < m; mx2++) {
                mappingUurlijst[mx1].push_back(idx);
                mappingUurlijst[mx2].push_back(idx);
                MappingUurlijstInvers[idx].assign({ mx1, mx2 });
                unions[idx].assign(n, false);
                unionVectors(matrix[mx1], matrix[mx2], unions[idx]);
                idx++;
            }
        }
        //mappingUur vullen.
        mappingUur.assign(n, {});
        for (size_t nx = 0; nx < n; nx++) {
            idx = 0;
            for (size_t mx1 = 0; mx1 < m; mx1++) {
                for (size_t mx2 = mx1 + 1; mx2 < m; mx2++) {
                    //checken of n in de doorsnede van 2 uurlijsten zit.
                    if (matrix[mx1][nx] && matrix[mx2][nx]) {
                        mappingUur[nx].push_back(idx);
                    }
                    idx++;
                }
            }
        }
        //initieel vullen van de combi2capacity: capacity van de doorsnede optellen
        combi2Capacity.assign(nrIds, 0);
        for (size_t nx = 0; nx < n; nx++) {
            if (capacity[nx] > 0) {
                for (size_t x : mappingUur[nx]) {
                    combi2Capacity[x] += capacity[nx];
                }
            }
        }
        //controleren of er tussen de combi2 lijsten onderling disjuncte zijn
        disjunctCombinations.assign(nrIds, {});
        mappingUurlijstSumDisjunct.assign(m, {});
        for (size_t idx1 = 0; idx1 < nrIds; idx1++) {
            disjunctCombinations[idx1].assign(2, 0); //de tellers voor zowel combi2 entries als minima entries
            disjunctCombinations[idx1][0]++;
            disjunctCombinations[idx1].push_back(idx1);
            mappingUurlijstSumDisjunct[MappingUurlijstInvers[idx1][0]].push_back(idx1);
            mappingUurlijstSumDisjunct[MappingUurlijstInvers[idx1][1]].push_back(idx1);
            for (size_t idx2 = idx1 + 1; idx2 < nrIds; idx2++) {
                if (!intersectionExists(unions[idx1], unions[idx2])) {
                    disjunctCombinations[idx1][0]++;
                    disjunctCombinations[idx1].push_back(idx2);
                    mappingUurlijstSumDisjunct[MappingUurlijstInvers[idx2][0]].push_back(idx1);
                    mappingUurlijstSumDisjunct[MappingUurlijstInvers[idx2][1]].push_back(idx1);
                    unionVectors(unions[idx1], unions[idx2], unions[idx1]);
                }
            }
        }
        for (size_t idx1 = 0; idx1 < nrIds; idx1++) {
            for (size_t mx = 0; mx < m; mx++) {
                if (!intersectionExists(unions[idx1], matrix[mx])) {
                    disjunctCombinations[idx1][1]++;
                    disjunctCombinations[idx1].push_back(mx);
                    mappingUurlijstSumDisjunct[mx].push_back(idx1);
                    unionVectors(unions[idx1], matrix[mx], unions[idx1]);
                }
            }
        }
        //initieel vullen van de combi2minima
        combi2Minima.assign(nrIds, 0);
        combi2MinimaSum.assign(nrIds, 0);
        for (size_t mx = 0; mx < m; mx++) {
            herberekenCombi2Minima(mx, minima);
        }
    }

    void herberekenCombi2Minima(const size_t& mx, const vector<int>& minima) {
        //mx is een aangepast minimum. Combi maken met elke andere mx2
        int min1, min2, cap12;
        size_t mx1, mx2;
        for (const size_t & idx : mappingUurlijst[mx]) {
            mx1 = MappingUurlijstInvers[idx][0];
            mx2 = MappingUurlijstInvers[idx][1];
            min1 = std::max(minima[mx1], 0); //enkel positieve nemen
            min2 = std::max(minima[mx2], 0); //enkel positieve nemen
            cap12 = combi2Capacity[idx];
            if (std::min(min1, min2) <= cap12) {
                //minstens een van beide valt binnen de beschikbare doorsnede
                combi2Minima[idx] = std::max(min1, min2);
            }
            else {
                //geen van beide valt binnen de beschikbare doorsnede
                combi2Minima[idx] = min1 + min2 - cap12;
            }
        }
        //extra..met sum erbij
        size_t offset;
        for (const size_t & idx : mappingUurlijstSumDisjunct[mx]) {
            offset = 2;
            combi2MinimaSum[idx] = 0;
            for (size_t idx1 = 0; idx1 < disjunctCombinations[idx][0]; idx1++) {
                combi2MinimaSum[idx] += combi2Minima[disjunctCombinations[idx][idx1 + offset]];
            }
            offset = 2 + disjunctCombinations[idx][0];
            for (size_t mx = 0; mx < disjunctCombinations[idx][1]; mx++) {
                combi2MinimaSum[idx] += minima[disjunctCombinations[idx][mx + offset]];
            }
        }
        grootsteMinimumDirty = true;
    }

    void vermeerderMetEen(const size_t& geraakteCapacity, const vector<size_t>& geraakteMinima, const vector<int>& minima) {
        //aanpassen combi2capaciteiten: allemaal + 1
        for (const size_t& x : mappingUur[geraakteCapacity]) {
            combi2Capacity[x]++;
        }
        //aanpassen combi2minima: herberekening
        for (const size_t& mx : geraakteMinima) {
            herberekenCombi2Minima(mx, minima);
        }
    }

    void verminderMetEen(const size_t& geraakteCapacity, const vector<size_t>& geraakteMinima, const vector<int>& minima) {
        //aanpassen combi2capaciteiten: allemaal - 1
        for (const size_t& x : mappingUur[geraakteCapacity]) {
            combi2Capacity[x]--;
        }
        //aanpassen combi2minima: herberekening
        for (const size_t& mx : geraakteMinima) {
            herberekenCombi2Minima(mx, minima);
        }
    }

    int grootsteCombi2Minimum() {
        if (grootsteMinimumDirty) {
            grootsteMinimum = 0;
            for (const int& cm : combi2MinimaSum) {
                grootsteMinimum = std::max(grootsteMinimum, cm);
            }
            grootsteMinimumDirty = false;
        }
        return grootsteMinimum;
    }
};

//---------------------------------------------------------------------------------------------------------------------------------------------------

struct CapacityMatrix {
    vector<vector<bool>> matrix;        //de uur/region lijsten
    vector<int> capacity;               //capaciteit van de uren/regions
    vector<int> minima;                 //te bereiken minima voor elke lijst
    vector<int> capacityLeft;           //capaciteit van de uren/regions die tijdens de berekening vermindert
    vector<int> minimaLeft;             //te bereiken minima voor elke lijst die tijdens de berekening vermindert
    Combi2MinimaCapacity* combi2Left;   //Methode2: hier gaat we de minima en capaciteiten bijhouden van 2 aan 2 uurlijsten. Enige doel: versnelling afbreken recursie.
    int nrClusters;                     //voor Methode2 kunnen we hiermee opsplistsen in subproblemen
    vector<CapacityMatrix*> clusters;   //voor Methode2 kunnen we hiermee opsplistsen in subproblemen
    bool overlap3InLijsten;             //indicatie dat Methode2 moet draaien (vanaf dat er minstens 3 in een cluster zitten)
    Bezetting bezetting;                //berekende minimum via methode1 en evt methode2.
    bool berekeningGedaan;              //indicatie dat de berekening klaar is
    uint64_t comboId;                   //combinatuie-id om te gebruiken in de map reedsBerekendeBezetting
    bitset<64> comboIdBitset;           //bitset voorstelling van het id. Kan nuttig zijn om bij door te geven aan de clusters indien nodig.
    vector<vector<bool>> regionMapping; //na reductie kan het nuttig zijn om eens te kijken hoe de nieuwe regions op de input regions/uren mappen.

    CapacityMatrix() : overlap3InLijsten(false), bezetting({0, 0, true}), nrClusters(1), comboId(0), comboIdBitset(0), berekeningGedaan(false), combi2Left(nullptr) {}
    CapacityMatrix(const vector<vector<bool>>& mat, const vector<int>& cap, const vector<int>& min) : bezetting({0, 0, true}) {
        matrix = mat;
        capacity = cap;
        minima = min;
        berekeningGedaan = false;
        nrClusters = 1;
        comboIdBitset = 0;
        for (size_t i = 0; i < mat.size(); i++) {
            comboIdBitset[i] = 1;
        }
        comboId = comboIdBitset.to_ullong();
        combi2Left = nullptr;
        bepaalOverlap3InLijsten();
        updateBerekendeWaardenAlsZeErAlZijn();
    }

    CapacityMatrix(const vector<vector<bool>>& mat, const vector<int>& min) : bezetting({ 0, 0, true }) {
        //default capacity = alles 1
        matrix = mat;
        capacity.assign(mat[0].size(), 1);
        minima = min;
        berekeningGedaan = false;
        nrClusters = 1;
        comboIdBitset = 0;
        for (size_t i = 0; i < mat.size(); i++) {
            comboIdBitset[i] = 1;
        }
        comboId = comboIdBitset.to_ullong();
        combi2Left = nullptr;
        bepaalOverlap3InLijsten();
        updateBerekendeWaardenAlsZeErAlZijn();
    }

    ~CapacityMatrix() {
        for (CapacityMatrix* x : clusters) {
            delete x;
        }
        if (combi2Left != nullptr) {
            delete combi2Left;
        }
    }

    void bepaalOverlap3InLijsten() {
        overlap3InLijsten = false;
        for (size_t n = 0; n < matrix[0].size() && !overlap3InLijsten; n++) {
            int tel = 0;
            for (size_t m = 0; m < matrix.size() && !overlap3InLijsten; m++) {
                if (matrix[m][n]) {
                    tel++;
                    overlap3InLijsten = (tel > 2);
                }
            }
        }
        if (overlap3InLijsten) {
            verdeelInClusters();
        }
    }

    void constructComboId(const bitset<64>& parentComboIdBitset, const vector<bool>& childCombo) {
        comboIdBitset = parentComboIdBitset;
        size_t p = 0;
        for (size_t i = 0; i < comboIdBitset.size(); i++) {
            if (comboIdBitset[i]) {
                comboIdBitset[i] = childCombo[p++];
            }
        }
        comboId = comboIdBitset.to_ullong();
    }

    void updateBerekendeWaardenAlsZeErAlZijn() {
        auto reedsBerekend = Bezetting::reedsBerekendeBezetting.find(comboId);
        if (reedsBerekend != Bezetting::reedsBerekendeBezetting.end()) {
            berekeningGedaan = true;
            bezetting = *(reedsBerekend->second);
        }
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
        outMatrix->constructComboId(comboIdBitset, combo);
        outMatrix->bepaalOverlap3InLijsten();
        outMatrix->updateBerekendeWaardenAlsZeErAlZijn();
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

    void verdeelInClusters() {
        //enkel nodig indien we methode2 gaan gebruiken.
        vector<int> clusterIds;
        int id = -1;
        clusterIds.assign(matrix.size(), -1);
        for (size_t n = 0; n < matrix[0].size(); n++) {
            for (size_t m = 0; m < matrix.size(); m++) {
                if (matrix[m][n]) {
                    if (clusterIds[m] == -1) {
                        clusterIds[m] = n;
                    }
                    else {
                        id = clusterIds[m];
                        for (size_t c = 0; c < clusterIds.size(); c++) {
                            if (clusterIds[c] == id) {
                                clusterIds[c] = n;
                            }
                        }
                    }
                }
            }
        }
        //hergroepeer de niet overlappende in 1 cluster omdat die toch samen in Methode1 kan.
        int singleClusterId = matrix[0].size(); // nog niet gebruikt id dat we aan de cluster van singles geven.
        bool uniqueId = true;
        for (size_t c1 = 0; c1 < clusterIds.size(); c1++) {
            uniqueId = true;
            for (size_t c2 = 0; c2 < clusterIds.size() && uniqueId; c2++) {
                uniqueId = (c1 == c2) || (clusterIds[c1] != clusterIds[c2]);
            }
            if (uniqueId) {
                clusterIds[c1] = singleClusterId;
            }
        }
        //is er nu meer dan 1 cluster? Indien niet heeft het opsplitsen geen zin en moeten we gaan met de volledige.
        uniqueId = true;
        for (size_t c = 1; c < clusterIds.size() && uniqueId; c++) {
            uniqueId = (clusterIds[0] == clusterIds[c]);
        }
        if (uniqueId) {
            nrClusters = 1;
        }
        else {
            nrClusters = 0;
            vector<bool> included;
            vector<bool> clusterCombo;
            included.assign(clusterIds.size(), false);
            for (size_t c1 = 0; c1 < clusterIds.size(); c1++) {
                if (!included[c1]) {
                    //start nieuwe cluster
                    nrClusters++;
                    clusterCombo.assign(matrix.size(), false);
                    for (size_t c2 = c1; c2 < clusterIds.size(); c2++) {
                        if (clusterIds[c2] == clusterIds[c1]) {
                            clusterCombo[c2] = true;
                            included[c2] = true;
                        }
                    }
                    //clusterCombo klaar.
                    clusters.push_back(reduceerCapacityMatrix(clusterCombo));
                }
            }
        }
    }

    int verminderMinimaMetEen1() {
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

    int verminderMinimaMetEen2(size_t& gekozenN, const vector<bool>& nMasker) {
        int returnVal = 0; //0: Ok maar nog niet opgelost. 1: opgelost (alle minima weggewerkt). -1: geen oplossing mogelijk
        int maxHits = 0;
        int maxN = -1;
        bool allesOpgelost = false;
        vector<int> hits;
        vector<size_t> geraakteMinima;
        hits.assign(capacityLeft.size(), 0);
        for (size_t n = 0; n < capacityLeft.size(); n++) {
            if (nMasker[n] && capacityLeft[n] > 0) {
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
            gekozenN = maxN;
            allesOpgelost = true;
            capacityLeft[maxN]--;
            for (size_t m = 0; m < minimaLeft.size(); m++) {
                if (matrix[m][maxN]) {
                    minimaLeft[m]--;
                    geraakteMinima.push_back(m);
                }
                allesOpgelost = allesOpgelost && (minimaLeft[m] <= 0);
            }
            combi2Left->verminderMetEen(gekozenN, geraakteMinima, minimaLeft);
            if (allesOpgelost) {
                returnVal = 1;
            }
        }
        else {
            returnVal = -1;
        }
        return returnVal;
    }

    void vermeerderMinimaMetEen2(const size_t& gekozenN) {
        vector<size_t> geraakteMinima;
        capacityLeft[gekozenN]++;
        for (size_t m = 0; m < minimaLeft.size(); m++) {
            if (matrix[m][gekozenN]) {
                minimaLeft[m]++;
                geraakteMinima.push_back(m);
            }
        }
        combi2Left->vermeerderMetEen(gekozenN, geraakteMinima, minimaLeft);
    }

    int bepaalMinimalePlaatsingMethode1() {
        minimaLeft = minima;
        capacityLeft = capacity;
        int iteraties = 0;
        int result = 0;
        do {
            result = verminderMinimaMetEen1();
            if (result != -1) {
                iteraties++;
            }
        } while (result == 0);
        return (result == 1) ? iteraties : -1;
    }

    void bepaalMinimalePlaatsingMethode2Sub(vector<bool> nMasker, int& maximum, int aantal, size_t& coutRecursions) {
        size_t gekozenN;
        int result = 0;
        //int grootsteMinimumNaAanpassing = 1;
        int grootsteMinimumNaAanpassing = combi2Left->grootsteCombi2Minimum();
        while ((aantal + grootsteMinimumNaAanpassing <= maximum) && (result != -1) && (coutRecursions < maxRecursion)) {
            result = verminderMinimaMetEen2(gekozenN, nMasker);
            if (result == 1) {
                //totale oplossing die beter is (kleiner dus) dan het huidige maximum
                maximum = aantal + 1;
                coutRecursions++; //eindpunt met oplossing.
            }
            if (result == 0) {
                //stap genomen maar niet klaar. We doen enkel verder als er nog een kans is op een totaal aantal < maximum
                //grootsteMinimumNaAanpassing = 1;
                grootsteMinimumNaAanpassing = combi2Left->grootsteCombi2Minimum();
                if (aantal + grootsteMinimumNaAanpassing < maximum) {
                    bepaalMinimalePlaatsingMethode2Sub(nMasker, maximum, aantal + 1, coutRecursions);
                }
                else {
                    coutRecursions++; //eindpunt zonder oplossing.
                }
            }
            if (result != -1) {
                //Aanpassing minima/capaciteit ongedaan maken en verder doen.
                vermeerderMinimaMetEen2(gekozenN);
                nMasker[gekozenN] = false;
            }
        }
    }

    int bepaalMinimalePlaatsingMethode2(int maximum) {
        //maximum is het minimum - 1 dat door bepaalMinimalePlaatsingMethode1 bepaald zal worden, is dus het startpunt dat verbeterd moet worden.
        //wordt enkel opgeroepen voor gevallen waarbij er 3 of meer overlaps van uurlijsten zijn.
        minimaLeft = minima;
        capacityLeft = capacity;
        vector<bool> nMasker;
        size_t coutRecursions = 0;
        nMasker.assign(capacity.size(), true);
        bepaalMinimalePlaatsingMethode2Sub(nMasker, maximum, 0, coutRecursions);
        if (coutRecursions >= maxRecursion) {
            bezetting.berekenbaar = false;
            if (TOONBEREKENING) {
                cout << endl << "NIET Berekenbaar: " << this->comboId << " Iterations: " << coutRecursions << "/" << maxRecursion;
                cout << endl;
            }
        }
        else {
            if (TOONBEREKENING) {
                cout << endl << "WEL Berekenbaar: " << this->comboId << " Iterations: " << coutRecursions << "/" << maxRecursion;
                cout << endl;
            }
        }
        return maximum;
    }

    void bepaalMinimalePlaatsing() {
        if (!berekeningGedaan && bezetting.berekenbaar) {
            if (nrClusters == 1) {
                bezetting.gevondenBezetting1 = bepaalMinimalePlaatsingMethode1();
                bezetting.gevondenBezetting2 = bezetting.gevondenBezetting1;
                if ((bezetting.gevondenBezetting1 != -1) && overlap3InLijsten) {
                    combi2Left = new Combi2MinimaCapacity(matrix, capacity, minima);
                    bezetting.gevondenBezetting2 = bepaalMinimalePlaatsingMethode2(bezetting.gevondenBezetting1 - 1);
                }
            }
            else {
                bezetting.gevondenBezetting1 = 0;
                bezetting.gevondenBezetting2 = 0;
                for (CapacityMatrix* x : clusters) {
                    if (bezetting.berekenbaar) {
                        x->bepaalMinimalePlaatsing();
                        bezetting.gevondenBezetting1 += x->bezetting.gevondenBezetting1;
                        bezetting.gevondenBezetting2 += x->bezetting.gevondenBezetting2;
                        bezetting.berekenbaar = bezetting.berekenbaar && x->bezetting.berekenbaar;
                    }
                }
            }
            Bezetting::reedsBerekendeBezetting.insert({ comboId, new Bezetting(bezetting.gevondenBezetting1, bezetting.gevondenBezetting2, bezetting.berekenbaar) });
            berekeningGedaan = true;
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

    vector<bool> combo;            //verwijzing naar de gecombineerde uurlijsten. Relatief tov input. Bij clusters tov combinatie.
    vector<bool> inverseUurlijst;  //inverse unie van de door combo gecombineerde uurlijsten
    int minimum;                   //het gevonden minimum voor deze combinatie (-1 indien geen oplossing gevonden en alles moet verworpen worden)
    int maximum;                   //is het totaal - minimum. Wordt het maximum voor de spreiding.
    int capaciteit;                //de capaciteit van inverseUurlijst. (aantal uren 1 in de lijst)
    bool essentieel;               //is het een elementaire combinatie (1 uit m). Moet steeds behouden blijven indien er teveel zouden zijn.
    bool berekenbaar;              //is de combinatie berekenbaar. indien niet, verwerpen.
    bool behouden;                 //combinatie weerhouden? Combinaties kunnen geïmpliceerd worden daar andere. Kunnen dus wegvallen zonder gevaar.

    Combinatie() : minimum(-1), maximum(-1), capaciteit(-1), essentieel(false), behouden(true), berekenbaar(true) {}
    Combinatie(vector<bool>& newCombo) : combo(newCombo), minimum(-1), maximum(-1), capaciteit(-1), essentieel(false), behouden(true), berekenbaar(true) {
        bepaalinverseUurlijst();
        //indien in de combo slechts één uurlijst staat dan is hij essentieel. Is ter bescherming omdat we niet weten welke combo's overleven door de maxOutputFactor.
        essentieel = (aantalTrue(combo) == 1);
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
        capaciteit = 0;
        for (size_t c = 0; c < inverseUurlijst.size(); c++) {
            if (inverseUurlijst[c]) { capaciteit++; }
        }
        //cout << endl;
        //coutBoolVector(inverseUurlijst);
    }

    static void voegCombinatiesVanBepaaldeGrootteToe(int grootte, size_t start, vector<bool>& comb, bool isEvenMidden, bool symmetrisch, bool flip) {
        if (grootte > 0) {
            for (size_t m = start; m < comb.size() && (Combinatie::combinaties.size() < maxCombinaties); m++) {
                comb[m] = true;
                Combinatie::voegCombinatiesVanBepaaldeGrootteToe(grootte - 1, m + 1, comb, isEvenMidden, symmetrisch, flip);
                comb[m] = false;
            }
        }
        else {
            if (symmetrisch) {
                if (start > 0) {
                    //bij start == 0 hebben we de combinatie waar NIETS gekozen is. Omgekeerde is dan wel goed, alles gekozen.
                    Combinatie::combinaties.push_back(new Combinatie(comb));
                }
                if (!isEvenMidden) {
                    //niet voor de middelste flippen als die er is.
                    comb.flip(); //omkeren
                    Combinatie::combinaties.push_back(new Combinatie(comb));
                    comb.flip(); //terug goed zetten
                }
            }
            else { // voor de versie klein naar groot
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
    }

    static void voegCombinatiesSymmetrischToe() {
        //best te gebruiken bij methode1 problemen
        size_t aantal = Combinatie::uurlijsten.size();
        size_t helft = div(aantal, 2).quot;
        bool even = (div(aantal, 2).rem == 0);
        vector<bool> comb;
        comb.assign(aantal, false);
        for (size_t g = 0; g <= helft && (Combinatie::combinaties.size() < maxCombinaties); g++) {
            Combinatie::voegCombinatiesVanBepaaldeGrootteToe(g, 0, comb, (helft == g) && even, true, false);
            if (TOONINPUT) {
                cout << endl << "Combinaties toevoegen (" << g << "," << (aantal - g) << "/" << aantal << ") ";
                cout << Combinatie::combinaties.size() << "/" << 2 * berekenCombinatie(g, aantal);
                cout << "/" << maxCombinaties << "/" << pow(2, aantal) - 1;
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
            Combinatie::voegCombinatiesVanBepaaldeGrootteToe(g, 0, comb, false, false, false);
            if (TOONINPUT) {
                cout << endl << "Combinaties toevoegen (" << g << "/" << aantal << ") ";
                cout << Combinatie::combinaties.size() << "/" << berekenCombinatie(g, aantal);
                cout << "/" << maxCombinaties << "/" << pow(2, aantal) - 1;
            }
        }
        for (size_t g = helft + 1; g <= aantal && (Combinatie::combinaties.size() < maxCombinaties); g++) {
            Combinatie::voegCombinatiesVanBepaaldeGrootteToe(aantal - g, 0, comb, false, false, true);
            if (TOONINPUT) {
                cout << endl << "Combinaties toevoegen (" << (g) << "/" << aantal << ") ";
                cout << Combinatie::combinaties.size() << "/" << berekenCombinatie(aantal - g, aantal);
                cout << "/" << maxCombinaties << "/" << pow(2, aantal) - 1;
            }
        }
    }

    static void voegCombinatiesToe() {
        if (Combinatie::regionMatrix->overlap3InLijsten) {
            voegCombinatiesVanKleinNaarGrootToe();
        }
        else {
            voegCombinatiesSymmetrischToe();
        }
    }

    static void toonCombinaties() {
        cout << endl << "Toegevoegde combinaties:" << endl;
        for (size_t c = 0; c < combinaties.size(); c++) {
            coutBoolVector(combinaties[c]->combo);
            cout << endl;
        }
    }

    static void berekenMinima() {
        //int nr1 = 0, nr2 = 0;
        if (TOONINPUT) {
            cout << endl << "Berekenen:(" << combinaties.size() << ")" << endl;
        }
        CapacityMatrix* comboMatrix;
        for (Combinatie* c : combinaties) {
            //output tonen
            if (TOONBEREKENING) {
                cout << ".";
            }
            comboMatrix = Combinatie::regionMatrix->reduceerCapacityMatrix(c->combo);
            comboMatrix->bepaalMinimalePlaatsing();
            c->minimum = comboMatrix->bezetting.gevondenBezetting2;
            c->berekenbaar = comboMatrix->bezetting.berekenbaar;
            delete comboMatrix;
            c->maximum = (c->minimum > 0) ? (Combinatie::totaal - c->minimum) : -1;
            if (c->maximum < 0) {
                ONMOGELIJK = true;
            }
        }
        if (TOONBEREKENING) {
            cout << endl;
        }
    }

    static void dumpCombinaties() {
        dump << std::endl;
        for (size_t c = 0; c < combinaties.size(); c++) {
            printBoolVectorStars(combinaties[c]->combo, dump);
            dump << '\t';
            printBoolVectorStars(combinaties[c]->inverseUurlijst, dump);
            dump << '\t' << combinaties[c]->maximum << '\t' << combinaties[c]->behouden;
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
                if (combinaties[c1]->behouden && isAEenSubSetVanB(combinaties[c1]->inverseUurlijst, combinaties[c2]->inverseUurlijst)) {
                    //c1 is subset van c2.
                    if (combinaties[c1]->maximum >= combinaties[c2]->maximum) {
                        combinaties[c1]->behouden = false;
                    }
                }
                else if (combinaties[c2]->behouden && isAEenSubSetVanB(combinaties[c2]->inverseUurlijst, combinaties[c1]->inverseUurlijst)) {
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

//---------------------------------------------------------------------------------------------------------------------------------------------------

void zoekOplossing() {
    Combinatie::setupCombinaties(MAT, MIN, MAX);
    Combinatie::voegCombinatiesToe();
    //Combinatie::toonCombinaties();
    Combinatie::berekenMinima();
    Bezetting::dumpReedsBerekendeBezetting();
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
    //MAX = 30; //min 30 nodig!
    //AANTAL = 20;
    //MAT.assign(AANTAL, {});
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
    
    zoekOplossing();
}

/*TODO
(ok) Indien er teveel combinaties overblijven, de meest geschikte x uitkiezen.
(ok) Input uurlijsten opkuisen indien er dezelfde zijn met een ander minimum.
(ok) In de combinaties clusters onderscheiden die apart opgelost kunnen worden.
(ok) in elke cm de de combo bijhouden in originele vorm, evnt als bitvector 64.
(ok) map (key, value) maken met deze ids als sleutel en de berekende waarde als resultaat.
(ok) berekende matrices bijhouden.
(ok) bij aanmaken cm eerst kijken of zelfde al berekend is geweest. Resultaat herbruiken.
(ok) ontouden of een cm al berekend is. dirty-flag.
(ok) aparte tabel bijhouden met resultaten
(ok) size_t oplossen
(ok) destructor maken die ook de subobjecten delete
(ok) reductie spreidingen ook kijken naar echte subset uurlijsten.
*/