DeciCrush - bit crusher / sample-rate reducer (VST3), creation originale
=======================================================================

Chaine de traitement (par canal) :
  Preamp -> Filter (si "Pre") -> Approx. Filter -> Resampler -> Quantizer
         -> Images Filter -> Filter (si "Post") -> FX Mix -> Output

Modules et parametres
- Preamp (0 a +48 dB)       : gain d'entree, pousse le signal dans le quantizer (ecretage type CAN)
- Quantizer
    Resolution (1-24 bits)  : nombre de niveaux = 2^bits ; interrupteur On/Off
    DC Shift                : On = mid-tread (niveau zero present), Off = mid-riser
    Dithering               : bruit TPDF avant quantification (100 % = 1 LSB)
- Resampler
    Frequency (44 Hz-44.1 kHz) : frequence d'echantillonnage cible (maintien d'echantillon) ; On/Off
    Jitter                  : variation aleatoire de la periode d'echantillonnage
    Approx. Filter + Frq. Deviation : passe-bas AVANT le resampler (Butterworth 6e ordre),
                               coupure = Nyquist cible x 2^deviation (en octaves)
    Images Filter + Frq. Shift : passe-bas APRES le resampler (Butterworth 8e ordre) qui retire
                               les images ; peut servir de passe-bas brickwall sans repliement
- Filter (LP / BP / HP / BR / Off)
    Cutoff, Reso / Width    : resonance (LP/HP) ou largeur de bande (BP/BR)
    Order Pre / Post        : avant ou apres le "crush"
- FX (0-100 %)              : dry/wet
- Output (-24 a +12 dB)
- Presets d'usine           : menu en haut de l'interface (et liste de programmes de l'hote)

Differences avec un produit commercial du meme type (a connaitre)
- Le resampler est un maintien d'echantillon sur la grille de l'hote : a un rapport non entier,
  les durees de maintien alternent (ex. 7 et 8 echantillons), ce qui ajoute une legere irregularite
  meme avec Jitter a 0. Pas de reechantillonnage sur-echantillonne.
- Pas de reglage "Quality", pas de MIDI Learn integre (utilise l'automation / le mapping MIDI de ton hote),
  pas de navigateur de presets avance, pas de themes ni de mise a l'echelle de l'interface.
- L'ordre des modules est un choix d'implementation.
