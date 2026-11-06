# Prova finale — Algoritmi e Strutture Dati 2025-2026
## Il Catering Impossibile

---

## Descrizione del problema

Organizzare una cena aziendale con tutti i dipendenti di una grande azienda.

- Il ristorante offre un insieme di **piatti**, ciascuno identificato da un nome univoco.
- Ogni dipendente ha espresso delle **richieste categoriche**: una lista di opzioni dove basta che **almeno una** venga soddisfatta.
  - Esempio: voglio la pizza *oppure* il sushi *oppure* che non ci sia il risotto.
- Perché la serata sia un successo, **tutti i dipendenti devono essere soddisfatti contemporaneamente**.

### Fallback gerarchico

Se non è possibile soddisfare tutti, si eliminano i dipendenti in ordine inverso di gerarchia (dall'ultimo al primo) finché non si trova una soluzione possibile.

---

## Formato dell'input

Il nome di un piatto segue le convenzioni di un identificatore C (es. `Risotto`, `pasta_al_pesto`).

```
<piatto1> <piatto2> ... <piattoN>
<richiesta dipendente 1 — rango più alto>
<richiesta dipendente 2>
...
<richiesta dipendente M — rango più basso>
```

- La **prima riga** elenca i piatti disponibili separati da spazio.
- Le righe successive elencano le richieste, **una per riga**, dal dipendente di rango più alto a quello di rango più basso.
- In ogni riga di richiesta:
  - `piatto` → il piatto è **gradito** (vuole che venga servito)
  - `-piatto` → il piatto è **sgradito** (vuole che non venga servito)
- Un dipendente è soddisfatto se **almeno uno** dei piatti graditi viene servito **oppure almeno uno** dei piatti sgraditi viene escluso.

---

## Formato dell'output

| Caso | Output |
|---|---|
| Tutte le richieste soddisfacibili | `OK` |
| Impossibile, si rimuove il dipendente *i*-esimo dal fondo | `-i` |
| Dopo aver rimosso abbastanza dipendenti si trova soluzione | `OK` |

L'output è quindi una sequenza di linee: prima `KO` se non si riesce subito, poi `-1`, `-2`, ... per ogni dipendente rimosso, finché `OK`.

---

## Esempi

### Esempio soddisfacibile

**Input:**
```
pizza sushi risotto torta
-pizza sushi
sushi risotto -pizza
```

**Output:**
```
OK
```

---

### Esempio insoddisfacibile

**Input:**
```
pizza sushi risotto torta
-pizza sushi -risotto
pizza -sushi risotto
-risotto
-sushi
risotto
sushi
```

**Output:**
```
KO
-1
-2
OK
```

**Spiegazione:**
- `KO` — le richieste di tutti e 7 i dipendenti non sono soddisfacibili.
- `-1` — si rimuove l'ultimo dipendente (rango 7) e si riprova: ancora impossibile.
- `-2` — si rimuove anche il penultimo (rango 6): ora la soluzione esiste.
- `OK` — con i dipendenti 1–5 si riesce.
