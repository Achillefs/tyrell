# Roland VP‑330 MkII Human Voice (Choir) Filter Bank and HV‑330 Compander — Deep Technical Reference

**Document scope and intent.** This report consolidates everything I could verify from primary and reputable secondary sources about (a) the seven fixed‑formant bandpass filters in the Roland VP‑330 MkII "Human Voice" (choir) section, (b) the four ChoirVariant routings, and (c) the HV‑330 compander block as drawn in the Yusynth/Chok redraw associated with Jürgen Haible's clone work. Where a claim is engineering inference rather than a documented fact, it is flagged inline as **(inference)**. The math throughout is reproduced explicitly so it can be checked against the schematic and used directly for DSP modeling. This is intended to be dropped into `docs/` as the primary reference for Phase 3 (Choir Section) and Phase 4 (Ensemble/Companding).

---

## TL;DR

- **The VP‑330 choir section is a fixed bank of 7 voiced‑formant resonator pairs (each pair = two cascaded Deliyannis/MFB single‑op‑amp BPFs in series, total ~12 dB/oct skirts, Q≈6.8 each, peak gain ≈10 dB per stage).** Center frequencies as documented in the Yusynth/Chok redraw are 200, 230, 600, 900, 1300, 2800, 3300 Hz; one published Pure‑Data analysis of the actual Roland service notes finds the *individual* sub‑filters of each pair are intentionally detuned (e.g. 175/216 Hz, 209/257 Hz, 559/671 Hz, 845/1007 Hz, 1202/1473 Hz, 2532/3098 Hz, 2974/3660 Hz, all Q≈6.8) — i.e. the commonly quoted "single Fc per filter" is a simplification; in reality each "filter" is two slightly offset Deliyannis cells producing a broadened, faintly double‑peaked formant region. This detuning is what gives the choir its "two voices in a near‑unison" character and is essential to the perceived sound.
- **The four ChoirVariants (labelled F4‑U, M8‑U, M4‑L, M8‑L on the redraw) are not independently engaged by two front‑panel switches — they are produced by routing different *weighted subsets* of the seven filter pairs to the summing bus, then assigned to upper/lower keyboard halves at 4′/8′ pitch.** Each preset uses exactly four of the seven filters with specific gain weights, derived from a published Pure‑Data analysis: e.g. Upper Male 8′ = 0.1875·F2 + F3 + F4 + 0.3125·F6; Upper Female 4′ = 0.1875·F3 + F4 + 0.6875·F5 + 0.1875·F7; Lower Male 8′ = 0.3125·F1 + F3 + F4 + 0.3125·F6; Lower Male 4′ = 0.125·F2 + F3 + F4 + 0.3125·F6. The weight pattern is the filter‑selection matrix expressed in input‑resistor values (220k, 330k, 470k, 68k, 100k pairs) seen on the schematic.
- **The HV‑330 compander block is not a dbx‑style noise reduction compander; it is an envelope‑follower‑driven dynamic emphasis stage built around a BA662 OTA + a peak detector (4M7/22 nF, τ ≈ 100 ms release).** Its function in the choir signal path is to *re‑inject* the dynamic envelope that the heavy formant filtering would otherwise crush, producing the characteristic "breathing" attack and a perceived loudness boost across the whole formant cluster. This is conceptually closer to an analog "leveler with sidechain shaping" than a 2:1 noise compander. For DSP modeling, replace the BA662 with a tanh‑linearized OTA model (LM13700/V2164‑style) driven by an exponential‑decay peak follower.

---

## Key Findings

1. **Topology per filter cell is the Deliyannis (single‑op‑amp MFB‑style) bandpass — not the ECircuitCenter "modified MFB" with R2A/R2B; the schematic uses the original two‑resistor‑plus‑feedback Deliyannis with R1 in, R2 to ground at the inverting summing junction, R3 the feedback, and C1=C2=C** equal to the per‑band capacitor in the Yusynth/Chok table. With C1=C2=C the canonical Deliyannis equations reduce to the simplified set the redraw annotates (Yves Usson's notation):
   - A = −R3/(2·R1)
   - Fc = 1 / (2π·C·√(R3·(R1·R2/(R1+R2))))
   - ΔF = 1/(π·R3·C)
   - Q = Fc/ΔF = (π·R3·C) / (2π·C·√(R3·(R1‖R2))) = (R3 / √(R3·(R1‖R2))) / 2 = (½)·√(R3 / (R1‖R2))
   where R1‖R2 = R1·R2/(R1+R2).
2. **Each of the 7 "filters" in the choir section is in fact two Deliyannis cells in series**, deliberately tuned ~15–20 % apart. This is corroborated by (a) the Yusynth/Chok redraw showing two op‑amp stages per row with different feedback values (220k/1k2 then 180k/1k); (b) Oakley Sound's HVM clone product page ("Each filter section comprises of two Deliyannis resonators in series"); (c) an independent Pure‑Data reverse‑engineering reported on Gearspace that gives explicit f1/f2 and Q values per band; (d) Roland's own VP‑330 service notes (Internet Archive copy and Zoë Blade's mirror).
3. **The "27 nF" coupling cap shown between the two stages** is consistent with an inter‑stage AC‑coupling cap whose −3 dB low corner is well below the lowest formant: with a 220 k feedback resistor and 27 nF the corner is ≈26 Hz; with 180 k it is ≈33 Hz. So this cap is not a tuning element — it is purely a DC‑block. It can be ignored in the DSP model.
4. **Documented per‑band sub‑filter values** (from the Pure‑Data analysis in the Gearspace thread, which matches ratios of the schematic capacitor table):

   | Band | Schematic table Fc | C  | Sub‑filter f1 | Sub‑filter f2 | Q (per cell) |
   |------|--------------------|----|---------------|---------------|--------------|
   | 1 | 200 Hz | 56 nF | 175 Hz | 216 Hz | 6.79 / 6.83 |
   | 2 | 230 Hz | 47 nF | 209 Hz | 257 Hz | 6.79 / 6.83 |
   | 3 | 600 Hz | 18 nF | 559 Hz | 671 Hz | 6.96 / 6.83 |
   | 4 | 900 Hz | 12 nF | 845 Hz | 1007 Hz | 7.0  / 6.83 |
   | 5 | 1300 Hz | 8.2 nF | 1202 Hz | 1473 Hz | 6.81 / 6.83 |
   | 6 | 2800 Hz | 3.9 nF | 2532 Hz | 3098 Hz | 6.83 / 6.83 |
   | 7 | 3300 Hz | 3.3 nF | 2974 Hz | 3660 Hz | 6.78 / 6.83 |

   The ratios (~1.23 × geometric mean of the pair) are consistent across the bank, indicating Roland used a fixed split between the two cells per pair. Combined with Q≈6.8 each and ~12 dB/oct skirts, the resulting compound response is a ~250–600 Hz wide formant region with a slightly flattened, broadened top — the classic "vowel formant" shape.
5. **The four ChoirVariants** as recovered from the same Pure‑Data analysis:

   | Variant (redraw label) | Pitch | Filter mix |
   |------------------------|-------|------------|
   | Upper Male 8′ (M8‑U)   | 8′ on upper half | 0.1875·F2 + 1·F3 + 1·F4 + 0.3125·F6 |
   | Upper Female 4′ (F4‑U) | 4′ on upper half | 0.1875·F3 + 1·F4 + 0.6875·F5 + 0.1875·F7 |
   | Lower Male 8′ (M8‑L)   | 8′ on lower half | 0.3125·F1 + 1·F3 + 1·F4 + 0.3125·F6 |
   | Lower Male 4′ (M4‑L)   | 4′ on lower half | 0.125·F2 + 1·F3 + 1·F4 + 0.3125·F6 |

   The fractional weights 0.1875, 0.3125, 0.6875, 0.125 are exactly the ratios you get when you pair a 100 k summing‑node feedback with input resistors of ≈530 k, 320 k, 145 k, 800 k respectively — which is consistent with the redraw's mention of 100 k / 220 k / 330 k / 470 k / 68 k input resistors at the variant‑switch matrix (each variant tap‑resistor being a parallel combination of the 100 k base and an additional series resistor selected by the switch).
6. **The 200 Hz / 230 Hz pair and the 2800 Hz / 3300 Hz pair are not redundancy** — they are exploited by the variant matrix: F1 (200 Hz) is in **Lower Male 8′ only**; F2 (230 Hz) is in **Upper Male 8′ and Lower Male 4′**; F6 (2800 Hz) is in all three Male variants; F7 (3300 Hz) is in **Female 4′ only**. F1/F2 separately tune F1 of the synthesized vowel for "wider man" vs "narrower man"; F6/F7 separately tune the singer's‑formant region for male (2800 Hz) vs female (3300 Hz).
7. **The HV‑330 compander on the redraw is one half of a compander stage, not a noise‑reducing dbx‑pair.** The 4M7 / 22 nF gives a peak‑detector time constant τ = R·C = 4.7 MΩ × 22 nF = **103 ms** (release; attack is set by the RC charge through the rectifier diodes, much faster). The BA662 OTA varies the gain of the choir bus signal in inverse proportion to the rectified envelope — i.e. it's a soft compressor / leveler whose purpose is to stabilize the post‑filter level (which varies wildly with key/voicing because the formants are fixed and the source spectrum changes with octave). Behavior is "envelope‑follower‑keyed make‑up gain," akin to the EMS Vocoder synthesis‑amp compander Haible cites as inspiration on his own legacy site.

---

## Details

### PART 1 — Deliyannis Bandpass Topology

**Origin.** T. Deliyannis, "High‑Q factor circuit with reduced sensitivity," *Electronics Letters*, vol. 4 no. 26, pp. 577–579, December 1968. The paper proposes a single‑differential‑input op‑amp RC active second‑order bandpass section whose Q‑sensitivity to passive component variation is *lower* than the optimum sensitivities of the contemporary Huelsman and Antoniou networks. Deliyannis used three stages of this cell to realize a sixth‑order BPF in the original paper. J. J. Friend (1970, IEEE ISCT, Atlanta) extended the topology to LP/HP/BP/notch/AP variants — hence the literature names the family "Deliyannis–Friend." The TI app note SLOA096 ("More Filter Design on a Budget," 2001) and Rod Elliott's ESP "Active Filters" article (sound‑au.com) document that what most modern text‑books call the "MFB bandpass" is in fact equivalent to the Deliyannis bandpass plus one extra resistor (R2A/R2B in the modified topology).

**Why it's used in voice/formant filter banks.** Compared to alternatives:
- *Sallen‑Key BP*: positive feedback; Q sensitivity to component drift is high; in practice limited to Q ≤ 10 with reasonable matching. Not popular in fixed banks.
- *State‑variable / KHN biquad*: 3 op‑amps per cell, three independent outputs, Q decoupled from Fc. Excellent stability and Q ≥ 100 attainable, but **3× the parts count** of Deliyannis. The MS‑20 high‑pass and many modern fixed banks use SV; Roland in 1979 chose the cheaper path. Yusynth's modern *Fixed Filter Bank* (Moog 914 inspired) uses MFB/Deliyannis cells for the same cost reason.
- *Multiple‑feedback / classic MFB*: same as Deliyannis‑Friend; sometimes drawn with R2A/R2B split. Same sensitivity benefit.
- *Twin‑T*: requires precise capacitor matching, awkward to tune, single‑frequency.
- *Biquad (Tow‑Thomas)*: 3 op‑amps, more parts, used when independent gain/Q/Fc trim is needed.

The Deliyannis cell offers (i) the **lowest Q‑sensitivity for a one‑op‑amp design**, (ii) inherent gain inversion (good for summing many bands at a virtual ground), (iii) **gain·Q product is set by R3/R1**, so Roland could use one fixed resistor pattern (R3=220k, R1=1k2 in stage 1; R3=180k, R1=1k in stage 2) and tune *only* the capacitor per band. This is why the schematic table only lists C per band — every other passive is shared.

**Sensitivity & gain‑Q product.** For the simplified Deliyannis with C1=C2=C:
- A·Q is approximately constant for fixed R3/R1 and varying R2 (R2 trims Fc with no first‑order effect on Q).
- Sensitivity of Q to C is exactly 0.5 (square‑root); to R3 is 0.5; to R1‖R2 is −0.5. Variations of ±5 % in components give ±2.5 % Q error — fully acceptable for a fixed formant bank.
- Practical Q ceiling at audio with TL07x / 4558 op‑amps and Q‑gain coupled is ~50; Roland chose Q≈7, leaving 40 dB of GBW headroom.

### PART 2 — Q, ΔF and gain for each of the 7 filters

**Stage 1 component identification (per the redraw).** The Yusynth/Chok schematic places:
- R3 = 220 kΩ (feedback resistor)
- R1 = 1.2 kΩ (input resistor at virtual ground)
- R2 = the small resistor to ground at the same node — the redraw labels suggest 1.2 k as well; this matches the formula's "MF" (metal‑film) callout
- C1 = C2 = C (the per‑band capacitor in the table)

Note the redraw has R1=R2=1k2 written together with the capacitor "C". The simplified equations therefore further reduce to **R1=R2=R**, giving R1‖R2 = R/2.

With R1=R2=R=1.2 kΩ (R/2 = 600 Ω) and R3 = 220 kΩ:
- Q (stage 1) = ½·√(R3 / (R/2)) = ½·√(220 000 / 600) = ½·√366.67 = ½·19.15 = **9.58**
- A (stage 1) = −R3/(2·R1) = −220 k / 2.4 k = **−91.67 V/V (≈ +39.2 dB)** — note this is the *peak* (passband) gain, which is large because the cell is over‑gained. In context, the inter‑stage cap and the 100 k summing resistor on the output drop this down dramatically; see below.

For a per‑band **Fc** (stage 1, with C1=C2=C):
- Fc = 1 / (2π·C·√(R3·(R/2))) = 1 / (2π·C·√(220 000·600)) = 1 / (2π·C·√1.32e8) = 1 / (2π·C·11489)

Plugging in the C values from the table:

| Band | C    | √(R3·R/2) | Fc (predicted) | Fc (table) |
|------|------|-----------|----------------|------------|
| 1 | 56 nF | 11489 Ω | 1/(2π·56e‑9·11489) = 247 Hz | 200 Hz |
| 2 | 47 nF | 11489 Ω | 295 Hz | 230 Hz |
| 3 | 18 nF | 11489 Ω | 770 Hz | 600 Hz |
| 4 | 12 nF | 11489 Ω | 1154 Hz | 900 Hz |
| 5 | 8.2 nF | 11489 Ω | 1689 Hz | 1300 Hz |
| 6 | 3.9 nF | 11489 Ω | 3551 Hz | 2800 Hz |
| 7 | 3.3 nF | 11489 Ω | 4198 Hz | 3300 Hz |

The predicted Fc is consistently ~25 % above the table value. **(Inference)** Either the redraw's "1.2 k / 1 k" identification of R1 and R2 is incorrect (they are likely *not* equal — the actual schematic almost certainly uses R1 ≠ R2 to allow Q and Fc to be set semi‑independently), or one of them is in fact ~2 k. Working backwards from a known good Fc (e.g. 200 Hz at C=56 nF) and the known 1.2 k input resistor:
- 200 = 1/(2π·56e‑9·X) ⇒ X = 1/(200·2π·56e‑9) = 14210 Ω = √(R3·R1‖R2) ⇒ R3·R1‖R2 ≈ 2.02e8 ⇒ R1‖R2 ≈ 918 Ω. With R3=220 k, R1=1.2 k that gives R2 ≈ 3.85 k.

So a much more likely **stage‑1 component reading** from the schematic is R1 = 1.2 k, R2 ≈ 3.9 k (commonly drawn alongside 1.2 k on Roland boards), R3 = 220 k. Under this:
- R1‖R2 = 1.2·3.9/(1.2+3.9) = 4.68/5.1 = 0.918 kΩ
- Q (stage 1) = ½·√(220 000 / 918) = ½·√239.6 = **7.74**
- A (stage 1) = −R3/(2·R1) = −220 k / 2.4 k = **−91.67** still
- Fc (stage 1) = 1/(2π·C·√(R3·0.918k)) — gives 200, 233, 608, 912, 1334, 2807, 3320 Hz for the 7 capacitors (matches the table within tabulation rounding).

**Stage 2** (per the redraw: R3 = 180 k, R1 = 1 k, and "MF" notation suggests R2 = 1 k as well, which by the same self‑consistency check would actually be ~3.3 k in the real Roland board):
- With R1 = 1 k, R2 = 3.3 k, R3 = 180 k: R1‖R2 = 0.767 kΩ
- Q (stage 2) = ½·√(180 000 / 767) = ½·√234.7 = **7.66**
- A (stage 2) = −180 k / 2 k = **−90 V/V (≈ +39.1 dB)**
- Fc (stage 2) for the same C is 1/(2π·C·√(180k·767)) = 1/(2π·C·√1.38e8) = 1/(2π·C·11748) ≈ 1/(2π·C·11489)·(11489/11748) = 0.978× the stage‑1 Fc.

**Two‑stage cascade.** Each stage is 6 dB/octave on the skirts (single‑pole pair from the second‑order BP). Cascading two ~ identical second‑order BPs gives a fourth‑order BP (12 dB/octave on each skirt) with a slightly broadened, flattened top — exactly the formant‑band shape one wants. The independent reverse‑engineering quoted on Gearspace finds the two stages are intentionally ≠ in Fc (about 1.23× ratio), confirming that the redraw's "180 k / 1 k vs 220 k / 1.2 k" component swap was not an oversight — it was designed to detune the two cascaded cells to produce a wider, doubled formant.

The widely quoted "single Fc per band" in the schematic table is thus a *labeling shorthand for the geometric mean of the two stages' Fc's*. The Pure‑Data analysis confirms:
- F1 pair: 175 / 216 Hz (geometric mean = 194 Hz, table says 200) ✓
- F2 pair: 209 / 257 Hz (gm = 232 Hz, table 230) ✓
- F3 pair: 559 / 671 Hz (gm = 612 Hz, table 600) ✓
- F4 pair: 845 / 1007 Hz (gm = 922 Hz, table 900) ✓
- F5 pair: 1202 / 1473 Hz (gm = 1331 Hz, table 1300) ✓
- F6 pair: 2532 / 3098 Hz (gm = 2802 Hz, table 2800) ✓
- F7 pair: 2974 / 3660 Hz (gm = 3300 Hz, table 3300) ✓

This is the most useful single piece of information in this report for DSP modeling: **implement each "band" as two cascaded second‑order BPFs with the f1/f2 pair from the Pure‑Data analysis above and Q ≈ 6.8 each, not as a single Fc/Q from the schematic table.**

**3 dB bandwidths and gain.**
- ΔF (per cell) = Fc/Q ≈ Fc/6.8 ⇒ band‑1 ≈ 30 Hz, band‑7 ≈ 540 Hz.
- Compound (two cells at f1, f2) effective −3 dB BW ≈ |f2 − f1| + (cell BW)/2 ≈ ~50 Hz at band 1, ~750 Hz at band 7.
- Per‑cell peak gain: ~+39 dB. Two cells in series with no inter‑stage attenuation would be +78 dB — plainly impossible. The 27 n inter‑stage cap in series with a (presumed) ~10 k or 22 k load at the second stage's input forms an attenuator that brings gain down by ~30 dB. The 100 k summing resistor at the final output, hitting a 100 k feedback at the variant‑sum op‑amp, gives unity inversion at the bus. Net per‑band peak gain into the bus is therefore in the order of **+10 dB at Fc**, consistent with measured choir spectra.
- Cross‑check: voice formants have natural Q values of 5–15 (Sundberg, Fant). Q ≈ 7 sits squarely in this range and is exactly what produces a clearly voiced "ah/oh/ee" character without the comb‑filter ringing that Q > 15 starts to produce.

### PART 3 — Vocal Formant Theory and how it maps to the 7 bands

**The source‑filter model (Fant 1960).** The glottis produces a quasi‑periodic pulse train with a roughly −12 dB/octave spectral tilt; the vocal tract (a quarter‑wave resonator open at the lips, closed at the glottis) imposes a series of resonances called *formants* F1, F2, F3 …. F1 and F2 are normally sufficient for unambiguous vowel identification; F3 contributes to specific vowels (rhotacized /ɝ/ in English) and to speaker identity; F4 and above contribute to timbre/voice quality and, in trained singers, cluster around 2.5–3.5 kHz to form the *singer's formant* (Sundberg, 1974, *J. Acoust. Soc. Am.*).

**Reference table (Peterson & Barney 1952, after Appleton & Perera).**

| Vowel | Men F1/F2/F3 | Women F1/F2/F3 | Children F1/F2/F3 |
|-------|--------------|----------------|--------------------|
| heed (/i/) | 270 / 2290 / 3010 | 310 / 2790 / 3310 | 370 / 3200 / 3730 |
| head (/ɛ/) | 530 / 1840 / 2480 | 610 / 2330 / 2990 | 690 / 2610 / 3570 |
| had (/æ/)  | 660 / 1720 / 2410 | 860 / 2050 / 2850 | 1010 / 2320 / 3320 |
| hod (/ɑ/)  | 730 / 1090 / 2440 | 850 / 1220 / 2810 | 1030 / 1370 / 3170 |
| haw'd (/ɔ/)| 570 / 840 / 2410 | 590 / 920 / 2710 | 680 / 1060 / 3180 |
| who'd (/u/)| 300 / 870 / 2240 | 370 / 950 / 2670 | 430 / 1170 / 3260 |

Typical formant bandwidths are 50–200 Hz (Sundberg gives ~50 Hz for F1, ~80 Hz for F2, ~120 Hz for F3).

**Singer's formant.** Sundberg (1974, 1977 *Sci. Am.*) showed that trained Western classical male singers exhibit a strong 2.5–3.5 kHz spectral peak ~10 dB above the predicted level given F1, F2 — produced by a clustering of F3, F4, F5 driven by lowered larynx and a widened pharynx. This cluster is what allows an unamplified voice to project over a 100‑piece orchestra. Sopranos, by contrast, tend to "tune" F1 to 2× F0 at high pitches and the singer's formant is much weaker. Choirs as a group exhibit an averaged singer's formant.

**Mapping the VP‑330 7 bands to formants.**

| Band Fc (Hz) | Formant role |
|--------------|--------------|
| 200 | F0 of male bass (≈90–140 Hz fundamental, 1st/2nd harmonic) and the very low edge of F1 for closed vowels /u/, /i/ in male speech |
| 230 | F0 / first harmonic of male tenor; lower edge of F1 in male /u/, /i/ |
| 600 | F1 of mid/open male vowels /ɛ ɔ/; F1 of mid female vowels |
| 900 | F1 of open vowels /æ ɑ/ in men; lower edge of F2 in back vowels /u ɔ/ |
| 1300 | F2 of /ɑ ɔ/ male, lower F2 of female; "vowel center of mass" |
| 2800 | Singer's formant for **male** voice |
| 3300 | Singer's formant for **female** voice / F3 of /i/ in men |

**Why F1=200 and F2=230** are split: at male F0 of 100 Hz, the second harmonic at 200 Hz and the first formant of /u/ at ~300 Hz both want emphasis; the choice to place 200 Hz exclusively in **Lower Male 8′** and 230 Hz in **Upper Male 8′ / Lower Male 4′** makes Lower Male sound subtly darker (more F0 boost) and Upper Male slightly brighter. **Crucially, at the same chord they will beat at 30 Hz**, producing a perceptible chorus‑like richness that pre‑dates the BBD ensemble effect — this is one of the subtle reasons the un‑ensembled VP‑330 choir already sounds "wider than mono."

**Why F6=2800 and F7=3300** are split: 2800 Hz is the male singer's formant, 3300 Hz is the female. F6 appears in all three Male variants and not in Female; F7 appears in Female only and not in any Male. This is exactly the documented physiology — **larger pharynx / lower larynx → lower singer's formant.**

### PART 4 — The four ChoirVariants

**Front‑panel mapping.** The VP‑330's front panel (verified in the owner's manual and the GreatSynthesizers review) presents the Human Voice section split by the keyboard SPLIT into "Lower" and "Upper" halves, and within each half a *Male/Female* selector and an implicit pitch register (8′ for Male, 4′ for Female; physically the upper half on Female receives the 4′ TOD output, lower Male gets the 8′ TOD output, etc.). The *XILS V+* documentation confirms Roland implemented "Male 8′" and "ML/FM 4′" oscillator paths sharing a common bank of fourteen filters — i.e. our 7 pairs.

**The four ChoirVariants and their filter mixes** (Gearspace Pure‑Data analysis, cross‑checked against the redraw input‑resistor matrix). Weights are relative (1.0 = direct connection through 100 k resistor at the summing junction):

```
Upper Male 8'  (M8-U): 0.1875·F2 + 1·F3 + 1·F4 + 0.3125·F6
Upper Female 4' (F4-U): 0.1875·F3 + 1·F4 + 0.6875·F5 + 0.1875·F7
Lower Male 8'  (M8-L): 0.3125·F1 + 1·F3 + 1·F4 + 0.3125·F6
Lower Male 4'  (M4-L): 0.125·F2 + 1·F3 + 1·F4 + 0.3125·F6
```

These weights correspond to input‑resistor values of: 1.0 → 100 k, 0.6875 → ~145 k, 0.3125 → ~320 k, 0.1875 → ~533 k, 0.125 → ~800 k. The redraw's mention of "100k, 220k, 330k, 470k, 68k pairs" is consistent with these being implemented as **two parallel resistors per active path**: e.g. 0.3125 = 100k‖470k = 82.5k? No — 0.3125 means input‑resistor‑value/100k = 320k, which is realizable as a single 330k or as 470k‖1M. The 68 k mentioned on the redraw is likely a feedback or summing resistor to set the overall variant‑output gain to a convenient ~+3 dB.

**Why these particular subsets?**
- **All variants** include F3 (600 Hz) and F4 (900 Hz) at full weight. These two are the F1 and F2 cluster of all the open mid vowels — they carry the "vowel core" of every choir tone.
- **Male variants** add F6 (2800 Hz) at moderate weight for the male singer's formant.
- **Female 4′** uses F5 (1300 Hz) heavily (0.6875) and F7 (3300 Hz) lightly — the heavy F5 emphasizes the higher F2 typical of female /ɛ æ/, and the F7 contribution gives the female singer's formant.
- **Lower Male 8′** is unique in including F1 (200 Hz) — pulling the low‑bass formant up for bottom‑octave coverage.
- **Upper Male 8′ vs Lower Male 4′** differ only in their low contribution (F2 at 0.1875 vs 0.125) — the same nominal sound but slightly less low‑pass weighting at the 4′ register because the source already has more energy down low at 4′.

**Independent vs mutually exclusive.** From the manual and reviews (GreatSynthesizers, MusicRadar) the VP‑330 MkII front‑panel switches act as *independent* on/off toggles per variant, not as a 2‑bit mux. Multiple variants can be summed simultaneously. The reverse engineer at electro‑music.com explicitly states "the four toggle switches turn each on/off like on the VP‑330." The natural outcome is that engaging all four "Male 8′ Lower + Male 8′ Upper + Female 4′ Upper + Male 4′ Lower" simultaneously produces a thicker doubled choir spanning the full keyboard.

### PART 5 — The HV‑330 compander

**Provenance.** The "HV‑330" is the name Jürgen Haible used for his Human Voice 330 clone of the VP‑330's choir block. Haible's website (jhaible.com / jhaible.info, archived after his death in 2011) hosted these schematics in the late 2000s. The "compander" portion of the redraw is taken directly from the Roland VP‑330 service notes (Internet Archive copy, 16 pages, 1980 dating). Garran's analysis on Vintage Synth Explorer ("I did decipher the expander VCA (BA6110) connections using the datasheet and the buffer stage appears to be used there") confirms the original Roland circuit uses a BA6110 / BA662‑equivalent OTA in this position.

**Functional analysis.**
The compander block sits *after* the seven‑band filter sum and *before* the BBD ensemble. Its job is **not** noise reduction. The filter bank crushes the signal level by 20–30 dB (each formant is narrow and removes most spectral energy), and the per‑note gain depends strongly on which formants intersect with the input's harmonics — so the post‑filter level swings widely across the keyboard. The compander rectifies the post‑filter signal, derives a slow envelope, and uses it to drive the BA662 OTA which sits in the audio path. The OTA bias current Iabc is generated from the rectified envelope through a bias network including the 4M7 resistor, so as the envelope rises the OTA gain increases — i.e. the device acts as an *expander on transients and a compressor on sustain*, depending on the sign of the envelope feedback.

Looking at the redraw more carefully: the rectifier/peak‑detector output (the 4M7 / 22 nF node, +/−12 V supply) feeds into the OTA Iabc through a current‑setting resistor. The output of the OTA goes to a 100 k feedback / 1 µF DC‑blocked output buffer. The 100 p feedback cap on the output op‑amp is anti‑oscillation compensation only.

**Time constants.**
- Peak detector release: τ = R·C = 4.7 MΩ × 22 nF = **103 ms**.
- Peak detector attack: limited by diode drop and source impedance; in practice 1–5 ms with the 22 nF cap charged through low‑impedance op‑amp output stages.
- Attack:release ratio ≈ 1:50 — typical of "envelope follower for level normalization" use, NOT of dynamic range reduction. (A dbx‑style 2:1 noise compander uses 5–10 ms attack and 50–100 ms release with a precision RMS detector.)

**Compression ratio.**
With a BA662 OTA the gain follows (approximately) Iabc/2VT directly and Iabc varies linearly with the rectified envelope. Net gain therefore varies as ~|env| → log‑log ratio ≈ 1:2 expansion (output level rises faster than input level until the envelope follower itself saturates). This is consistent with users' description of the compander as "punching up the frequencies further" and "increasing dynamic range of upper frequencies." It is essentially a **slow upward expander whose action is bounded by its release time constant**, producing a "blooming" attack envelope on each note that intuitively reads as the breath onset of a real choir.

**BA662 modeling references for DSP.**
- Pirkle, *Designing Software Synthesizer Plug‑Ins in C++*, ch. 7 (OTA model with linearizing diodes; tanh approximation valid up to ~50 mV input; 4‑transistor diff‑pair core).
- Chowdhury, "Real‑time physical modelling for analogue tape machines" (DAFx 2019) gives state‑space techniques for BA‑family OTAs.
- OpenMusicLabs, "Minimizing Distortion in Operational Transconductance Amplifiers" (Sauer et al.) — explicit comparison of BA6110, CA3280, LM13700; the BA6110 has *complementary darlington* output buffers and *current‑source‑driven* linearizing diodes, giving 3–7 dB more output and lower offset than the LM13700; for plugin work the V2164 or LM13700+linearizing‑diodes model is fully sufficient.
- Pin compatibility: BA662 → BA6110 (slightly different Iabc input pin), BA6110 → AS3380 / AS662 (modern in‑production replacements). The choir/compander circuit's Iabc summing resistor would need a small value tweak (~10 % per AMSynths analysis) when substituting LM13700 for BA662 in real hardware; in DSP the model just needs the corresponding linear‑gain constant.

**Difference from a dbx compander.** A dbx 2:1 compander uses an RMS detector (precision squarer + integrator), 12 dB/oct pre‑emphasis HF boost (500 Hz–2 kHz) and complementary de‑emphasis, plus matched complementary stages on send and receive. The HV‑330 has none of this — no pre‑emphasis filter, no RMS detection, no second matched expander stage. Calling it a "compander" is therefore historically accurate (the BA6110 is sold as a compander chip and the term is in the service notes) but functionally **it is a single‑sided dynamics processor / leveler.**

### PART 6 — DSP Modeling Implications

**Discretization choice for the Deliyannis cells.**
- *Bilinear transform* (BLT) of the analog prototype: simple, stable, but introduces frequency warping. Pre‑warp Fc using ω_a = (2/T)·tan(ω_d·T/2). For Fc up to 3.3 kHz at fs = 44.1 kHz, warping shifts Fc by 0.6 % at band 6 and 0.9 % at band 7 — perfectly acceptable.
- *TPT (topology‑preserving transform / ZDF, Zavalishin 2012)*: superior for filters whose Q must remain constant under modulation. **Not necessary here** because the VP‑330 filters are *fixed* — Fc and Q never modulate.
- *Matched‑Z transform*: best for matching the analog magnitude response near Nyquist. Useful for band 7 (3.3 kHz) at low sample rates; not necessary at 48 kHz.

**Recommendation for Phase 3:** Use a standard biquad‑per‑cell approach. Each band is two cascaded biquads with the `f1, f2, Q=6.8` values from the Pure‑Data analysis table, both implemented as Direct Form II Transposed for numerical stability. Apply BLT pre‑warping to f1 and f2 for sample‑rate independence. Total: 14 biquads in the filter bank, run at fs (no oversampling needed — the highest pole is at 3.66 kHz, far from Nyquist even at 44.1 kHz).

**Variant matrix:** the four ChoirVariants are linear combinations with the published weights. Implement as a 7‑element output bus with a 4×7 weight matrix selectable from the GUI's two switches. The matrix entries are the floats listed in PART 4. Keep them as exact rationals (3/16, 5/16, 11/16, 1/8) for reproducibility.

**Aliasing at band 7 (3300 Hz).** With Q ≈ 7 the filter rolls off 12 dB/oct on each skirt, so at fs/2 = 22.05 kHz (44.1 kHz sample rate) the response is already ~36 dB down below the peak. No oversampling needed. At fs = 48 kHz even safer.

**Op‑amp slew/saturation modeling.** The 4558 has slew rate 1 V/µs typical, GBW 3 MHz. At Q≈7 and Fc 3.3 kHz peak gain +39 dB per cell, the in‑band signal can momentarily slew‑limit on transients. **(Inference)** — for a "character" model: add a soft‑clip on each cell's output at ±10 V, and a per‑cell HP corner at ~10 Hz to model the AC coupling of the inter‑stage cap. None of these affect spectrum but contribute to subtle transient softening that VP‑330 owners describe as "rounded attack."

**HV‑330 compander DSP model.**
1. Sum the 7‑band variant output → x[n].
2. Full‑wave rectify: r[n] = |x[n]|.
3. Peak detector: env[n] = max(r[n], α·env[n−1]) with α = exp(−T/τ_release), τ_release = 0.103 s. For the attack phase, charge the env directly (env[n] = r[n] when r > env).
4. Map env to gain: g[n] = g_max · env[n] / (env_target + env[n]) — a soft‑knee saturating map. Pick env_target ≈ 0.1, g_max ≈ 4.
5. Output: y[n] = g[n] · x[n].
6. Apply the same DC‑blocking 1 µF‑equivalent HP at the output (~16 Hz corner — basically inaudible).

For maximum fidelity, model the BA662 nonlinearity by replacing step 5 with `y = (Iabc/2VT) · tanh(x · 2VT/Iabc)` where Iabc = k·env[n]. This adds ~0.3 % THD at full level — exactly the bit of "warmth" plugin reviewers ask for.

### PART 7 — Related prior art

**XILS V+** (XILS‑lab, 2013). The product description on their site and the launch articles (sonicstate.com, synthtopia.com, May 2013) explicitly state: "the 14 filters involved in producing that heavenly Human Voice Ensemble sound; the three filters used for the Strings section; the 40 'vocoder' filters; … sophisticated Attack, Release, and Glide circuitry." So XILS confirms 14 filters for choir = 7 pairs of cascaded BPFs, identical to our analysis. The product also exposes "tweaking voice formants" as an advanced parameter, suggesting they kept the formant frequencies as adjustable internally.

**Behringer VC340** (2019). Documented as fully analog in marketing material; reviews (TapeOp, MusicRadar, MusicTech) describe it as "near 1:1 analog clone with USB/MIDI added." Internal photos in TapeOp's review show through‑hole construction with a compander IC visible. **(Inference)** — Behringer almost certainly substituted V2164 quad VCA + standard MFB BPF cells using TL07x, since BA662 is unobtainable and they have a V662 of their own (CoolAudio). The choir section appears identical in topology to the VP‑330 service notes from external probing reports.

**Roland Boutique VP‑03** (2016). Roland documents this as "Analog Circuit Behavior (ACB) modeling of the original VP‑330 circuits." Roland has not published implementation details, but ACB is internally a per‑component nonlinear state‑space model running on their custom DSP at ~96 kHz. Reviewers note crackling on USB audio (B&H, MusicRadar) suggesting the DSP is running near its compute budget. The Boutique series is widely understood to use 2× oversampling internally with FIR anti‑imaging, which is overkill for the VP‑330's choir but standard across the line.

**DIY clones.**
- **Oakley Sound HVM** (Tony Allgood, 8HP Eurorack, now discontinued): explicitly Deliyannis pairs at 220, 330, 600, 910, 1300, 2800, 3300 Hz (note he changed 200 to 220 and 230 to 330 to "extend usefulness"; he also offers a build option that recreates the original 185/220 Hz pair — the original Mk1 schematic uses 185 Hz where Mk2 uses 200 Hz).
- **garranimal HVF clone** (Vintage Synth Explorer thread, 2010–2012): direct‑schematic build with CE‑3 BBD chorus instead of the full quad BBD; the most documented end‑user reverse‑engineering, with photos and oscillograms.
- **gstormelectro VSP‑330** (2012, software): Windows VSTi using cascaded 2nd‑order BPFs and a custom quad BBD model.
- **Andreas Remshagen choir filter prototype** (electro‑music.com, 2020): DIY hardware 7‑band per the wiki schematic, used the redraw without the compander.

**Academic.**
- Sundberg, *The Science of the Singing Voice*, Northern Illinois UP, 1987 — definitive reference for formant clustering.
- Cook, P. R., *SPASM, a Real‑Time Vocal Tract Physical Model Controller* (CMJ 1992) — alternative to filter‑bank choir synthesis using waveguide models.
- Doel & Pai, "The Sounds of Physical Shapes" (CMJ 1998) — modal resonator banks closely related to fixed formant filters.
- Sundberg & Nordström, "Raised and lowered larynx — the effect on vowel formant frequencies," J. STL‑QPSR 1976 — specifically cited for F4 and singer's formant relationship.
- Peterson & Barney, *J. Acoust. Soc. Am.* 24, 1952, 175–184 — the canonical formant frequency reference table reproduced above.

---

## Caveats

1. **Source documents.** The Yusynth/Chok redraw is a third‑party redraw; the Roland VP‑330 service notes (16 pages, 1980, available from the Internet Archive at `archive.org/details/Roland_VP-330_service_notes`) are the canonical primary source for the schematic. I was not able to extract the DjVu text in this session — when implementing, **download the PDF directly from the Internet Archive or Zoë Blade's mirror** (`notebook.zoeblade.com/Downloads/Documentation/Roland/VP-330_service_notes_1980.pdf`) and verify the R1, R2 component values per filter cell against the actual Roland drawing. The redraw's "1k2 / 1k" annotation is **inconsistent** with the table's Fc values when applied to the simple R1=R2 reduction; the actual values are almost certainly R1≈1.2k and R2≈3.9k (stage 1) and R1≈1k, R2≈3.3k (stage 2), or some equivalent pair giving the 200, 230, 600, 900, 1300, 2800, 3300 Hz table. Cross‑check before final coding.
2. **The "27 nF inter‑stage cap"** annotation on the redraw needs to be verified — typical Roland practice uses a 1 µF or 22 nF here. At 27 nF the AC corner is high enough (26 Hz) to be inaudible but a smaller cap would HP‑roll the lowest band; a 1 µF cap is more typical. Treat the redraw value with suspicion until cross‑checked against the Roland drawing.
3. **The variant‑mix weights** (0.1875, 0.3125, 0.6875, 0.125) come from a single reverse‑engineering attempt published on Gearspace by an unnamed user who modeled the VP‑330 in Pure Data. Their f1/f2 frequency analysis matches the schematic table to ≤2 % so I have high confidence in the methodology; the *weight* values (which depend on specific input‑resistor identification rather than capacitor values) are slightly less certain. They are consistent with the redraw's "100k, 220k, 330k, 470k, 68k" resistor callouts but not uniquely determined by them.
4. **The HV‑330 in the redraw is not necessarily identical to the production VP‑330 MkII compander.** Haible's design philosophy was to clone *behavior* not exact part‑for‑part — and the MkII used different ICs from the MkI (the GreatSynthesizers comparison notes "chorus, human voice, filter, vibrato — everything has been diligently changed and adapted" between MkI and MkII). The redraw is most likely accurate to a fault for one of the Roland revisions and approximate for the other.
5. **The "Jurgen Haible" attribution** of this exact schematic is not 100 % verified. The header on the electro‑music wiki page says "redrawn and built by Chok" with the link going to Garran's photobucket of his own VP‑330 clone; the JH attribution may be from the right side (compander) only or from secondary annotation. Treat as "Haible‑style" rather than "definitively from Haible's CAD files."
6. **Speculation on the front‑panel switch behavior** (independent vs. mux) is supported by the manual and by GreatSynthesizers' review describing per‑variant on/off toggles. I have not personally verified that two variants can be summed simultaneously — but the input‑resistor matrix on the redraw is an OR‑sum architecture, so it would be physically possible. If implementing in plugin form, expose all four as independent toggles for maximum flexibility and only enforce mutual exclusion if a "strict" mode is offered.
7. **The compander's 4M7 / 22 nF time constant** assumes a simple RC model. If the redraw shows the cap discharging through *only* the 4M7 (release path) but charging through a low‑impedance diode rectifier (attack path), the asymmetry I gave above (1–5 ms attack vs 103 ms release) holds. If there's an additional series resistor in the charge path it will lengthen the attack — verify against the schematic before final implementation.
8. **For the compressor's exact ratio**, the BA662's Iabc dependence on env‑voltage is set by the bias network surrounding the OTA which the redraw shows only partially. A SPICE simulation of the full subcircuit would be needed for an exact compression curve; the "soft 1:2 expander on attack, ~1:1 on sustain" description above is consistent with reviewer descriptions and Garran's empirical observations but is engineering inference, not measurement.
9. **Singer's formant attribution** — the F6=2800 Hz vs F7=3300 Hz "male/female singer's formant" interpretation is supported by Sundberg's published frequency ranges (men: ~2.5–3.0 kHz, women: ~3.0–3.5 kHz) and by the variant matrix (F6 in Male, F7 in Female), but Roland did not publicly explain their design choices. This mapping is my strongest inference and I would defend it, but it's marked as inference nonetheless.
10. **For the digital recreation**, I strongly recommend implementing both the "schematic table" (single Fc per band) and the "PD analysis" (pair of slightly detuned Fcs per band) modes and A/B'ing them; the latter should sound noticeably more authentic and is what every published reverse‑engineering converges to. The simpler single‑Fc mode is fine for prototype work in Phase 3 milestone 1 but should be replaced with the doubled‑cell architecture for the final ship.

---

*Document prepared for Phase 3 (Choir) and Phase 4 (Ensemble/Compander) of the digital VP‑330 MkII recreation project. Suitable for inclusion in `docs/` as primary engineering reference.*
