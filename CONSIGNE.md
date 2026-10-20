# Détection de contours

L’objectif de ce TP est de comparer 3 détecteurs de gradient (Sobel, Laplace, Canny) avec une série d’images. Dans ces méthodes, il vous faudra fixer des seuils ou paramètres. Ces valeurs ne seront certainement pas adaptées à toutes vos images. L’ajout de « trackbar » à vos fenêtres vous ferra certainement gagner un peu de temps. Le site web d’OpenCV propose quelques exemples d’utilisateur de ces détecteurs qui pourront vous aider.

Vous devrez rédiger un rapport dans lequel vous présenterez une évaluation quantitative des détecteurs. Pour cela, il vous faudra comparer les images de contours obtenues à partir de votre programme avec des images de contours de référence (des images où les contours ont été tracés «à la main»). Vous trouverez ce type de données à l’adresse suivante : http://www.cs.rug.nl/~imaging/databases/contour_database/contour_database.html

Pour comparer deux « images de contours »,  5 grandeurs vous seront utiles :
```
    contours_détectés : nombre de pixels contours dans l’image calculée 
    contours_référence : nombre de pixels contours dans l’image de référence 
    contours_corrects : nombre de pixels contours correctement détectés dans l’image calculée
    selon l’image de référence (aussi égal à contours_détectés ∩ contours_référence) 
    faux_positifs : nombre de pixels détectés comme contours, mais non contours dans l’image
    de référence (aussi égal à contours_détectés – contours_corrects) 
    faux_négatifs : nombre de pixels contours non détectés dans l’image calculée, mais contour dans l’image de référence (aussi égal à contours_référence – contours_corrects)
``` 
A partir de ces 5 grandeurs, vous calculerez 3 mesures pour évaluer les détecteurs :
```
    La performance :
    P = contours_corrects / (contours_corrects + faux_positifs + faux_négatifs) 
    Le taux de faux positifs :
    TFP = faux_positifs / (contours_corrects + faux_positifs + faux_négatifs)
    Le taux de faux négatifs :
    TFN = faux_ négatifs / (contours_corrects + faux_positifs + faux_négatifs)
```
Il est demandé d’écrire un programme :
```
    Permettant de choisir le détecteur
    Permettant de régler les paramètres associés
    Capable de traiter « un lot d’images » 
```
Enfin, vous commenterez vos résultats (bonne détection, bonne localisation, «meilleure méthode», variation en fonction des paramètres d’entrée, méthode utilisée pour la comparaison [atout/faiblesse …], … etc …) en illustrant avec quelques images de votre choix.

Attention, pour le calcul de «contours_corrects», il est plus parfois plus judicieux de déterminer la présence d’un pixel dans l’image de référence en utilisant un voisinage (3×3 est suffisant) plutôt qu’uniquement sa position dans l’image dans l’image des contours.

A RENDRE Un dossier TP_contours contenant  :
```
    Votre code source qui permet de calculer les mesures présentées ci-dessus sur un lot d’image.
    Un rapport au format PDF contenant au moins les éléments suivants :
        Présentation théorique des détecteurs utilisés
        Explications des fonctions d’OpenCV utilisées. En particulier cette explication devra faire le lien entre la présentation des détecteurs et les paramètres des fonctions.
        Tableau résumant les valeurs des mesures calculées.
        Interprétation des résultats obtenus et comparaison des trois détecteurs.
        Une rapide vidéo (3 à 5 minutes) qui explique votre programme et commente les résultats.
```

les critères d’appréciation de votre travail :
```
    Code & Interface : fonctionnalité des 3 détecteurs (Sobel, Laplace, Canny), présence des trackbars pour ajuster les paramètres, et capacité à traiter un lot d'images (batch).
    Évaluation quantitative : calcul correct des 5 métriques de base et des 3 mesures finales (Performance, FPR, FNR) avec une gestion du voisinage. Éventuellement, proposition d'autres critères d'évaluation.
    Rapport écrit : présentation théorique, explication des fonctions OpenCV, tableau récapitulatif des mesures et qualité des interprétations/comparaisons des résultats.
    Vidéo explicative : démonstration claire du programme et commentaires pertinents sur les résultats (durée de 3 à 5 minutes).
```