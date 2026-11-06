#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NBUCKETS 6701
#define NON_DECISO -1
#define ESCLUSO     0
#define SERVITO     1


typedef struct Nodo {
    char *nome;
    int id;
    struct Nodo *next;
} Nodo;

// un'opzione = un piatto + lo stato che il dipendente vuole (servito o escluso)
struct opzione {
    int piatto;
    int stato;
};

struct riga_dep {
    int da;
    int quanti;
    int ndip;
};

// tokenizer

int leggi_token(char *buf, int *segno)
{
    int c;
    int i = 0;
    *segno = 1;

    c = getchar();
    // salta bianchi
    while (c==' ' || c=='\t') c = getchar();

    if (c=='\n')
        return 0;
    if (c==EOF) return -1;

    if (c=='-'){
        *segno = -1;
        c = getchar(); }

    while (c!=' ' && c!='\t' && c!='\n' && c!=EOF) {
        buf[i] = (char)c;
        i++;
        c = getchar();
    }
    buf[i] = '\0';

    // quando va a capo lo legge la chiamata dopo e chiude la riga
    if (c=='\n')
        ungetc(c, stdin);

    return 1;
}

// hash table

static Nodo *tabella[NBUCKETS];

static unsigned trova_slot(const char *s)
{
    unsigned h = 0;
    int i = 0;
    while (s[i] != '\0') {
        h = (h * 67 + s[i]) % NBUCKETS;
        i++;
    }
    return h;
}

int inserisci(const char *nome){
    static int prossimo_id = 1;
    unsigned c = trova_slot(nome);
    Nodo *p = tabella[c];
    while(p!=NULL) {
        if (strcmp(p->nome, nome)==0)
            return p->id;
        p = p->next;
    }

    Nodo*nuovo = malloc(sizeof(Nodo));
    nuovo->nome = malloc(strlen(nome) + 1);
    strcpy(nuovo->nome,nome);
    nuovo->id = prossimo_id;
    prossimo_id = prossimo_id + 1;
    nuovo->next = tabella[c];
    tabella[c] = nuovo;
    return nuovo->id;
}

int cerca(const char *nome){
    unsigned c = trova_slot(nome);
    Nodo *p = tabella[c];
    while (p!=NULL){
        if (strcmp(p->nome, nome)==0)
            return p->id;
        p = p->next;
    }
    return -1;
}

    // var globali

static struct opzione *opzioni = NULL;
static int nval = 0;
static int capval = 0;

static struct riga_dep *righe = NULL;
static int nrighe = 0;
static int caprighe = 0;

static int P = 0;
static int D = 0;

/* contatori per decidere se vale la pena la scorciatoia
   sulle istanze grosse con tante dipendenze corte */
static int nCorte = 0;
static int n_lunghe = 0;

static int *prova = NULL;

static int *visto_pos = NULL;
static int *visto_neg = NULL;

// per ogni piatto mi segno in quali richieste compare
static int **dove_pos = NULL;
static int **dove_neg = NULL;
static int *quante_p = NULL;
static int *quante_n = NULL;
static int *cap_p = NULL;
static int *cap_n = NULL;
// peso per la scelta, le richieste corte pesano di piu
static int *peso_p = NULL;
static int *peso_n = NULL;
static int *coda = NULL;  // coda dei controlli da fare
static int qcap = 0;
static int testa = 0;
static int fondo = 0;

static int *lavagna = NULL;
static int *quadernetto = NULL;
static int nquad = 0;
static int *backup = NULL;
static int k_ok = 0;
static int *ricordo = NULL;   // l'ultimo valore visto per ogni piatto
static long nodi_visita = 0;
static long cap_nodi = 0;   // se >0 fermo la visita dopo tot nodi




void carica_dati(void)
{
    char buf[512];
    int segno, r, id, inizio, salta;

    //  il menu
    while (leggi_token(buf, &segno) == 1) {
        inserisci(buf);
        P++;
    }

    visto_pos = calloc(P + 1, sizeof(int));
    visto_neg = calloc(P + 1, sizeof(int));
    lavagna   = malloc((P + 1)* sizeof(int));
    quadernetto = malloc((P + 1) * sizeof(int));
    dove_pos = calloc(P + 1, sizeof(int *));
    dove_neg = calloc(P + 1, sizeof(int *));
    quante_p = calloc(P + 1, sizeof(int));
    quante_n = calloc(P + 1, sizeof(int));

    int k;
    for (k=1; k<=P; k++)
        lavagna[k] = NON_DECISO;

    cap_p= calloc(P +  1, sizeof(int));
    cap_n= calloc(P +  1, sizeof(int));
    backup= malloc((P +  1)* sizeof(int));
    ricordo= malloc((P +  1)* sizeof(int));

    for (k=1; k<=P; k++) {
        backup[k] = NON_DECISO;
        ricordo[k] = NON_DECISO;
    }
    prova = malloc((P + 1)* sizeof(int));
    int sentinella = 1;
    while (sentinella) {
        r = leggi_token(buf, &segno);
        if (r==-1) sentinella = 0;
        else if (r==1) {
        D++;
        if (peso_p == NULL) {
            peso_p = calloc(P +  1, sizeof(int));
            peso_n = calloc(P +  1, sizeof(int)); }
        inizio = nval;
        salta = 0;

        memset(visto_pos, 0, sizeof(int)* (P + 1));
        memset(visto_neg, 0, sizeof(int)* (P + 1));

        while (r==1 && !salta){
            id = cerca(buf);
            if (segno==1) {
                if (visto_neg[id])
                    salta = 1;
                else if (!visto_pos[id]) {
                    visto_pos[id] = 1;
                    if (nval==capval) {
                        struct opzione *tmp;
                        if (capval==0)
                            capval = 1067;
                        else
                            capval = capval* 2;
                        tmp = realloc(opzioni, capval* sizeof(struct opzione));
                        opzioni = tmp;
                    }
                    opzioni[nval].piatto = id;
                    opzioni[nval].stato = SERVITO;
                    nval++;
                }
            } else {
                if (visto_pos[id])
                    salta = 1;
                else if (!visto_neg[id]) {
                    visto_neg[id] = 1;
                    if (nval==capval) {
                        struct opzione *tmp;
                        if (capval==0)
                            capval = 1067;
                        else
                            capval = capval* 2;
                        tmp = realloc(opzioni, capval* sizeof(struct opzione));
                        opzioni = tmp;
                    }
                    opzioni[nval].piatto = id;
                    opzioni[nval].stato = ESCLUSO;
                    nval++;
                }
            }
            r = leggi_token(buf, &segno);
        }

        // se ero uscito per taut finisco di leggere la riga
        while (r == 1)
            r = leggi_token(buf, &segno);
         if (salta){
            nval = inizio;
        } else {
            if (nrighe==caprighe) {
                struct riga_dep *tmp;
                if (caprighe==0)
                    caprighe = 267;
                else
                    caprighe = caprighe * 2;
                tmp = realloc(righe, caprighe * sizeof(struct riga_dep));
                righe = tmp;
            }
            righe[nrighe].da = inizio;
            righe[nrighe].quanti = nval - inizio;
            righe[nrighe].ndip = D;

            // richieste corte pesano di piu nella scelta
            int len = nval - inizio;
            int p_riga = 67 / len;

            if (len == 2) nCorte = nCorte + 1;
            if (len > 3)  n_lunghe = n_lunghe + 1;

            // aggiungo questa richiesta alle liste dei piatti che contiene
            int z;
            for (z=inizio; z<nval; z++) {
                int v = opzioni[z].piatto;
                if (opzioni[z].stato==SERVITO)
                    peso_p[v] = peso_p[v] + p_riga;
                else peso_n[v] = peso_n[v] + p_riga;
                if (opzioni[z].stato==SERVITO) {
                    if (quante_p[v]==cap_p[v]) {
                        if (cap_p[v]==0)
                            cap_p[v] = 4;
                        else cap_p[v] = cap_p[v] * 2;
                        dove_pos[v] = realloc(dove_pos[v], cap_p[v] * sizeof(int));
                    }
                    dove_pos[v][quante_p[v]] = nrighe;
                    quante_p[v] = quante_p[v] + 1;
                } else {
                    if (quante_n[v]==cap_n[v]) {
                        if (cap_n[v]==0) cap_n[v] = 4;
                        else cap_n[v] = cap_n[v]*2;
                        dove_neg[v] = realloc(dove_neg[v], cap_n[v] * sizeof(int)); }
                    dove_neg[v][quante_n[v]] = nrighe;
                    quante_n[v] = quante_n[v] + 1;
                }
            }

            nrighe++;
        }

        if (r==-1) sentinella = 0;
        }
    }
}

static void assegna(int id, int v)
{
    lavagna[id] = v;
    quadernetto[nquad] = id;
    nquad = nquad + 1;
    ricordo[id] = v;   // !!mi segno qui polarita!
}

// clean assegnazioni fino a pos
static void annulla_da(int pos)
{
    while (nquad > pos) {
        nquad = nquad - 1;
        lavagna[quadernetto[nquad]] = NON_DECISO;
    }
}


// incoda tutte le richieste (entro m) che contengono il piatto v
static void accoda_var(int v, int m)
{
    int k;
    int *lp = dove_pos[v];
    int np = quante_p[v];
    for (k=0; k<np; k++) {
        int cidx = lp[k];
        if (cidx < m) {
            if (fondo==qcap) { qcap = qcap*2; coda = realloc(coda, qcap*sizeof(int)); }
            coda[fondo] = cidx;
            fondo = fondo + 1;
        }
    }
    lp = dove_neg[v];
    int nn = quante_n[v];
    for (k=0; k<nn; k++) {
        int cidx = lp[k];
        if (cidx < m) {
            if (fondo==qcap) { qcap = qcap*2; coda = realloc(coda, qcap*sizeof(int)); }
            coda[fondo] = cidx;
            fondo = fondo + 1;
        }
    }
}


/* smaltisce la coda e sistema le richieste con un solo piatto libero.
   appena ne trova una senza scampo svuota la coda e ritorna 0 */
static int propaga(int m)
{
    while (testa < fondo) {
        int ci = coda[testa];
        testa = testa + 1;
        int j = righe[ci].da;
        int fine = righe[ci].da + righe[ci].quanti;
        int indecisi = 0;
        int ult_p = 0, ult_s = 0;
        int sodd = 0;
        while (j<fine && !sodd) {
            int pid = opzioni[j].piatto;
            int st = opzioni[j].stato;
            if (lavagna[pid]==st) sodd = 1;
            else if (lavagna[pid]==NON_DECISO) {
                indecisi++;
                ult_p = pid;
                ult_s = st;
            }
            j = j + 1;
        }
        if (!sodd) {
            if (indecisi==0) { testa = 0; fondo = 0; return 0; }
            if (indecisi==1) { assegna(ult_p, ult_s); accoda_var(ult_p, m); }
        }
    }
    testa = 0;
    fondo = 0;
    return 1;
}




static unsigned randomizer(void)
{
    static unsigned mescola = 0xBE77067u; //seed betto67
    mescola = mescola * 8191u + 16767u;
    mescola = mescola ^ (mescola >> 13);
    return mescola;
}




/* provo a tirare su un menu a caso: lavagna vuota, propago le obbligate,
   poi sui piatti ancora liberi a caso e propago. se non si puo
   ricomincio da capo. dopo un po' di tentativi sbagliati stoppa. */
static int costruisci_casuale(int m){
    int tent = 0;
    int trovato = 0;
    int buttati = 0;   // quanti ne ho buttati,  sui test peggiori bastavano

    int max_tent = 2700;
    while (tent<max_tent && trovato==0) {
        int i;
        for (i=1;i<=P;i++) lavagna[i] = NON_DECISO;
        nquad = 0;
        if (qcap < m+64) { qcap = m+256; coda = realloc(coda, qcap*sizeof(int)); }
        testa = 0; fondo = 0;
        for (i=0;i<m;i++){
            coda[fondo]=i;
            fondo++; }

        int conflitto = 0;
        if (!propaga(m)) return 0;   // gia' impossibile cosi

        int continua = 1;
        while (continua && conflitto==0) {
            // prendo piatto libero partendo da un punto a caso
            int start = 1 + (int)(randomizer() % (unsigned)P);
            int v = 0;
            int passo = 0;
            while (passo<P && v==0) {
                int idx = start + passo;
                if (idx>P) idx = idx - P;
                if (lavagna[idx]==NON_DECISO) v = idx;
                passo++;
            }
            if (v==0)
                continua = 0;         // tutti decisi
            else {
                if (randomizer() & 1u) assegna(v, SERVITO);
                else assegna(v, ESCLUSO);
                accoda_var(v, m);
                if (!propaga(m)) conflitto = 1;
            }
        }
        if (conflitto==0) {
            for (i=1;i<=P;i++) ricordo[i] = lavagna[i];
            trovato = 1;
        } else buttati++;
        tent = tent + 1;
    }
    (void)buttati;
    return trovato;
}


// visita in profondita: prova un valore, se fallisce prova l'altro
static int visita(int m)
{
    if (cap_nodi>0 && nodi_visita>cap_nodi){
        return -1;
    }
    nodi_visita++;
    if (!propaga(m)) {
        return 0;
    }
    /* scelgo il piatto non ancora deciso con peso piu alto
       (le corte valgono +) */
    int id = 0;
    int meglio = -1;
    int v;
    for (v=1; v<=P; v++) {
        if (lavagna[v]==NON_DECISO) {
            int tot= peso_p[v] + peso_n[v];
            if (tot> meglio) {
                meglio= tot; id = v;
            }
        }
    }
    if (id==0) return 1;  //  deciso

    // mi segno fino a dove sono arrivato nel quadernetto, cosi posso tornare
    int salvato = nquad;
    int primo, secondo;
    /* se ho gia visto questo piatto prima riuso la scelta, altrimenti
       provo prima il valore che compare  di piu nelle richieste corte */
    if (ricordo[id]==SERVITO || (ricordo[id]==NON_DECISO && peso_p[id]>=peso_n[id])) {
        primo= SERVITO;
        secondo= ESCLUSO;
    } else {
        primo= ESCLUSO;
        secondo= SERVITO;
    }

    assegna(id, primo);
    accoda_var(id, m);
    int r1 = visita(m);
    if (r1==1) return 1;
    annulla_da(salvato);
    if (r1<0) return -1;
    assegna(id, secondo);
    accoda_var(id, m);
    int r2 = visita(m);
    if (r2==1) return 1;
    annulla_da(salvato);
    if (r2<0) return -1;
    return 0;
}

static void risolvi(void)
{
    int ci, i;
    int da_dove = 0;
    k_ok = D;

    /* le scorciatoie le provo solo sulle istanze grosse.
       lunghezza media delle richieste = nval/nrighe */
    if (nrighe > 230) {
        if (n_lunghe > 0 && nval * 25 < nrighe * 69) {
            // richieste in media corte (sotto 2.76) ma con qualcuna lunga:
            // qui meglio tirare a caso e propagare
            if (costruisci_casuale(nrighe)) {
                k_ok = D;
                return;
            }
        } else if (nrighe* 10 <P* 31) {
            /* poche richieste per piatto:
               una sola risoluzione completa basta. limite sui nodi per sicurezza. */
            for(i=1; i<=P; i++) lavagna[i]= NON_DECISO;
            nquad = 0;
            if (qcap < nrighe + 64){
                qcap = nrighe + 256;
                coda = realloc(coda, qcap * sizeof(int));
            }
            testa = 0; fondo = 0;
            for (ci=0; ci<nrighe; ci++) { coda[fondo] = ci; fondo = fondo + 1; }
            nodi_visita = 0;
            cap_nodi = (long)nrighe * 250;
            int rfull = visita(nrighe);
            cap_nodi = 0;
            if (rfull==1) {
                for (i=1; i<=P; i++) {
                    if (lavagna[i]==NON_DECISO){
                        if (peso_p[i]>=peso_n[i]) backup[i] = SERVITO;
                        else backup[i]=ESCLUSO;
                    } else backup[i]=lavagna[i];
                }
                k_ok = D;
                return;
            }
            for (i=1; i<=P; i++) ricordo[i] = NON_DECISO;
        }
    }


    int fermato = 0;
    ci = da_dove;
    while (ci<nrighe && fermato==0) {
        int ok = 0;
        int fine = righe[ci].da + righe[ci].quanti;
        int jb = righe[ci].da;
        while (jb<fine && ok==0) {
            int pid = opzioni[jb].piatto;
            if (backup[pid]==opzioni[jb].stato) ok = 1;
            jb = jb + 1;
        }

        if (ok == 0){
            // ricomincio da zero
            for (i=1; i<=P; i++)
                lavagna[i] = NON_DECISO;
            nquad = 0;
            // espande coda e popola con le richieste 0 ci
            if (qcap<ci + 1 + 128) {
                qcap= ci + 1 + 256;
                coda= realloc(coda,qcap*sizeof(int)); }
            testa = 0;
            fondo = 0;
            int kk;
            for(kk=0;kk<=ci;kk++){
                coda[fondo]= kk;
                fondo=fondo+1;
            }
            int impossibile=0;
            if (!propaga(ci+1)) impossibile = 1;

            //se un piatto tra le richieste non ancora accontentate compare con un solo segno lo posso fissare
            if (!impossibile) {
                int cambiato = 1;
                while (cambiato) {
                    cambiato = 0;
                    int v;
                    for(v=1; v<=P; v++){
                        visto_pos[v]=0;
                        visto_neg[v]=0;
                    }
                    int c2;
                    for (c2=0; c2<=ci; c2++){
                        int fine = righe[c2].da + righe[c2].quanti;
                        int sodd = 0;
                        int j = righe[c2].da;
                        while (j<fine && sodd==0) {
                            int pid = opzioni[j].piatto;
                            if (lavagna[pid]==opzioni[j].stato) sodd = 1;
                            j= j + 1;
                        }
                        if (sodd==0) {
                            for (j=righe[c2].da; j<fine; j++) {
                                int pid = opzioni[j].piatto;
                                if (lavagna[pid]==NON_DECISO) {
                                    if (opzioni[j].stato==SERVITO) visto_pos[pid] = 1;
                                    else visto_neg[pid] = 1;
                                }
                            }
                        }
                    }
                    for (v=1; v<=P; v++) {
                        if (lavagna[v]==NON_DECISO) {
                            if (visto_pos[v] && !visto_neg[v]) {
                                assegna(v, SERVITO);
                                cambiato = 1;
                            } else if (visto_neg[v] && !visto_pos[v]) {
                                assegna(v, ESCLUSO);
                                cambiato = 1;
                            }
                        }
                    }
                }
            }

            int trovato = 0;
            if (!impossibile) trovato = visita(ci+1);
            if (trovato) {
                for (i=1;i<=P;i++) backup[i]=lavagna[i];
            } else{
                k_ok = righe[ci].ndip - 1;
                fermato = 1;
            }
        }
        ci = ci + 1;
    }
}

int main(void)
{
    carica_dati();
    risolvi();

    int R = D - k_ok;
    if (R==0) {
        puts("OK");
    } else {
        int i;
        puts("KO");
        for (i=1; i<=R; i++)
            printf("-%d\n", i);
        puts("OK");
    }

    return 0;
}
