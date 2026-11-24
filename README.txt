AtelierDSP - 3 plugins VST3 originaux : SlapTape Echo, DeciCrush, HeatSat.

Compilation : poussez ce dossier sur GitHub, l'onglet Actions compile macOS et Windows.
Installation macOS (cible 14.4 ou plus) : copiez les .vst3 voulus dans ~/Library/Audio/Plug-Ins/VST3/ puis, pour chaque bundle :
  xattr -dr com.apple.quarantine "<bundle>.vst3"
  codesign --force --deep -s - "<bundle>.vst3"
Presets utilisateur : ~/Documents/AtelierDSP/<NomDuPlugin>/*.xml
