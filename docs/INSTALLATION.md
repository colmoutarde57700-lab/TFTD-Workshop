# Installation / premier démarrage
Windows x64. Extraire le ZIP dans un dossier où vous pouvez écrire, puis lancer l'EXE. L'INI et les projets restent locaux.
Pour mettre à jour une installation existante, garder une copie de l'ancien EXE et de TFTD_Workshop.ini ; ajouter le nouvel EXE au même dossier. Aucun installateur ne modifie le jeu.

Ressources → TFTD ORIGINAL : dossier contenant vos ressources originales TFTD.
Ressources → OXCE STANDARD : ressources standard/xcom2 de votre installation.
Ressources → MODS OXCE : user/mods.
Les chemins ne sont pas fournis par la release. Aucun MCD/PCK/TAB/MAP/PNG commercial n'est inclus.

Langage : FR/EN/ES/DE. Tuto : guide complet hors ligne.
Rendu → Dossier des gabarits universels : racine du pack personnel, Resources, TFTD_HD ou Terrain.
Exemple : MonPack/Resources/TFTD_HD/Terrain/SAND/015.png.
Le nom PNG représente le numéro d'image Frame[0], pas forcément l'index MCD.

Pour le rendu mixte, choisir REAL HD normal/debug, puis PNG avec REAL HD → Remastered/Gabarits universels.
REAL HD normal recherche TFTD_REAL_HD_TEXTURES ; debug recherche TFTD_REAL_HD_DEBUG sous MODS, dans Resources/TFTD_HD/RealHD/Datasets/SAND/Materials.
TOP_BASE.png et VERTICAL_BASE.png donnent le sol et les faces. Les tuiles GEO utilisent aussi ces matériaux.
La release n'inclut pas ces packs. Le guide décrit leur fonctionnement et les différences avec les PNG 512×640.

PNG invisible : vérifier racine, dataset, Frame[0], dimensions et alpha. Texture REAL HD absente : vérifier les chemins MODS et Materials.
Tester une seule texture avant de remplacer un pack. Garder vos gabarits acceptés à part des essais.
