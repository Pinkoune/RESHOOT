# RESHOOT — Game Design Document

Nov 12, 2026 · @Jérémy Barcelo

## Fiche d'identité

RESHOOT est un FPS solo où le joueur est l'acteur principal d'un film dont le réalisateur change les règles en plein combat.

|  |  |
| --- | --- |
| Titre | RESHOOT |
| Genre | FPS solo d'arène, contre des bots |
| Thème de la jam | Director's Cut |
| Moteur | Unreal Engine 5.9, C++ |
| Plateforme | PC Windows, clavier et souris |
| Durée d'une partie | 10 à 15 minutes |
| Joueurs | 1 |
| Équipe | 4 personnes, développement assisté par IA |
| Références | The Finals (ton de jeu télévisé), Apex Legends (déplacement) |

**Pitch.** Tu es la vedette d'un film qui change de genre toutes les cinq minutes. Chevaliers, drones, gangsters : les figurants t'attaquent pendant qu'un réalisateur tyrannique crie « CUT! », t'impose ses caprices et note ta performance. À la fin, il monte sa propre version, où tous les genres se mélangent.

## Piliers de design

Quatre règles tranchent toute décision de design pendant la jam.

1. **Le réalisateur commande.** Chaque système doit pouvoir être perturbé par lui : règles, décor, arme, gravité. Une fonctionnalité qu'il ne peut pas toucher passe après les autres.
2. **Tout est un tournage.** Rien n'est « vrai » : les murs sont des panneaux, les ennemis sont des figurants, la mort est une prise ratée. Cela justifie les décors simples et supprime le game over.
3. **Une seule prise en main.** Mêmes touches et même déplacement dans tous les univers. Seuls l'arme et, dans l'espace, la gravité changent.
4. **Compréhensible seul, en 30 secondes.** Le juge joue sans nous. Chaque règle est dite par la voix off et affichée à l'écran.

## Boucle de jeu et structure

La boucle de base est la **prise** : une vague d'ennemis de 60 à 90 secondes, interrompue une fois par le réalisateur.

1. **Clap.** Le réalisateur annonce la prise, les figurants entrent.
2. **Combat.** Le joueur élimine les figurants, l'audimat monte.
3. **« CUT! »** Tout se fige. Le réalisateur impose une directive.
4. **« ACTION! »** Le combat reprend avec la contrainte. La respecter rapporte plus.
5. **Fin de prise.** Dernier figurant éliminé : commentaire du réalisateur, prise suivante.

**Répétition, puis mise en scène.** La première prise de chaque scène se joue dans un décor en carton, bricolé et visiblement faux. À la fin de cette prise, le réalisateur crie « On améliore la mise en scène ! » : une transition balaie le plateau et remplace chaque objet par sa version réaliste, à la même place. Les prises suivantes se jouent dans le beau décor. L'arme du joueur et les costumes des figurants passent du carton au réaliste en même temps. Le gameplay ne change pas : murs, couverts et chemins restent identiques.

Si le joueur meurt, la prise recommence au clap, avec une pénalité d'audimat. Il n'y a pas de game over.

Une partie complète :

| Étape | Contenu | Durée visée |
| --- | --- | --- |
| Menu | Clap géant, bouton ACTION | — |
| Casting | Fond vert, tutoriel des déplacements et du tir | 1 min |
| Scène 1, médiéval | 2 à 3 prises | 3 min |
| Scène 2, spatial | 2 à 3 prises | 3 min |
| Scène 3, film noir | 2 à 3 prises | 3 min |
| Final « Director's Cut » | L'univers change toutes les 20 secondes | 2 min |
| Écran de fin | Affiche du film, étoiles | — |

Entre deux scènes, le décor change à vue pendant que le réalisateur parle : c'est le seul temps mort, il dure moins de 15 secondes.

La difficulté monte par la densité des vagues et par des directives plus contraignantes, pas par des ennemis plus résistants.

## Contrôles et déplacement

Le déplacement s'inspire d'Apex Legends : rapide, basé sur l'élan, avec une glissade. Il est codé une seule fois dans le personnage joueur et il est identique en médiéval et en film noir.

| Touche | Au sol (médiéval, film noir) | En apesanteur (spatial) |
| --- | --- | --- |
| ZQSD | Se déplacer | Se déplacer dans la direction du regard |
| Souris | Viser | Viser |
| Maj | Sprint | Boost |
| Espace | Saut | Monter |
| Ctrl | S'accroupir ; glissade si on sprinte | Descendre |
| Clic gauche | Action principale de l'arme | Idem |
| Clic droit | Action secondaire (bouclier, visée) | Idem |
| R | Recharger | Idem |

Règles de déplacement :

- **Glissade.** S'accroupir en sprintant lance une glissade : friction très faible et impulsion vers l'avant. Elle dure 1,2 seconde au plus.
- **Saut glissé.** Sauter pendant une glissade conserve l'élan, avec un léger bonus de vitesse.
- **Contrôle aérien élevé.** Le joueur peut corriger sa trajectoire en l'air.
- **Pas de dégâts de chute.**
- **Apesanteur.** Mêmes vitesses maximales qu'au sol, gravité nulle, freinage faible : le joueur dérive s'il lâche les touches.

Valeurs actuelles dans le code, à régler en jeu (unités Unreal, 100 = 1 mètre) :

| Paramètre | Valeur |
| --- | --- |
| Vitesse de marche | 600 par seconde |
| Vitesse de sprint | 950 par seconde |
| Vitesse accroupi | 300 par seconde |
| Impulsion de saut | 620 |
| Contrôle aérien | 0,8 |
| Vitesse minimale pour glisser | 700 par seconde |
| Impulsion de glissade | 450 |
| Durée maximale de glissade | 1,2 s |
| Délai entre deux glissades | 0,4 s |
| Bonus du saut glissé | × 1,1 |
| Accélération en apesanteur | 1 500 |

L'escalade des rebords est optionnelle et ne sera pas commencée avant samedi.

## Armes

Le joueur a une seule arme par univers, imposée par le réalisateur : pas d'inventaire, pas de ramassage. Il a 100 points de vie, qui remontent à 100 au début de chaque prise. Un tir à la tête double les dégâts.

Les valeurs ci-dessous sont des propositions de départ, à régler en jeu.

| Univers | Arme | Clic gauche | Clic droit | Dégâts | Cadence, munitions |
| --- | --- | --- | --- | --- | --- |
| Médiéval | Épée et bouclier | Coup d'épée dans un cône de 2,5 m devant soi | Lever le bouclier : dégâts de face réduits à 20 % dans un cône de 120° | 50 par coup | 1,5 coup par seconde |
| Médiéval | Arc (touche 2 ou molette) | Maintenir pour bander, relâcher pour tirer | — | 40 à 100 selon la charge | Charge complète en 1 s, flèches illimitées |
| Spatial | Fusil laser | Tir instantané en rafale continue | Visée (zoom léger) | 12 par tir | 8 tirs par seconde, surchauffe après 3 s de tir continu |
| Film noir | Revolver | Tir instantané, coup par coup | Visée (zoom léger) | 45 par balle | 6 balles, rechargement de 1,5 s |

Règles communes :

- Bouclier levé, le joueur ne peut ni frapper ni sprinter.
- Le laser ne se recharge pas : il surchauffe, puis refroidit en 1,5 seconde. C'est sa différence avec le revolver.
- Pendant le final, l'arme change en même temps que l'univers, avec chargeur plein.
- Une mitraillette d'époque pour le film noir est optionnelle et ne sera pas commencée avant samedi.

## Ennemis

Les ennemis sont des figurants : ils sont nombreux, fragiles et prévisibles, et chacun n'a qu'un seul comportement. Valeurs de départ à régler.

| Univers | Ennemi | Comportement | Vie | Dégâts | Vitesse |
| --- | --- | --- | --- | --- | --- |
| Médiéval | Chevalier figurant | Court vers le joueur et frappe au contact | 100 | 20 par coup, 1 coup par seconde | 700 par seconde |
| Médiéval | Archer | Immobile sur une position haute, tire une flèche lente et visible | 50 | 25 par flèche, 1 tir toutes les 2,5 s | Fixe |
| Spatial | Drone | Vole en ligne droite vers le joueur, tire à moins de 15 m | 40 | 8 par tir, 2 tirs par seconde | 500 par seconde |
| Film noir | Gangster | Rejoint un couvert, se découvre pour tirer, change de couvert quand on l'approche | 80 | 15 par balle, rafale de 3 toutes les 2 s | 450 par seconde |

Règles communes :

- **Gel au « CUT! ».** Tous les ennemis s'arrêtent net, tirs et déplacements compris, puis reprennent à « ACTION! ».
- **Les chevaliers courent plus vite que la marche du joueur** (700 contre 600), mais moins vite que son sprint : il faut bouger pour leur échapper.
- **Les drones n'utilisent aucun calcul de chemin.** Le décor spatial doit donc rester ouvert au centre.
- **Mort théâtrale.** Un figurant éliminé s'effondre de façon exagérée, puis est évacué du plateau.
- **Vagues.** 4 à 6 ennemis en même temps au début, 8 à 10 dans les dernières prises. Ils entrent par les portes du plateau, jamais dans le dos du joueur à moins de 10 m.

## Le réalisateur

Le réalisateur est une voix off qui dirige la partie, impose des directives et note le joueur par l'audimat.

**Personnalité.** À décider en équipe. Proposition : un tyran de plateau persuadé de tourner un chef-d'œuvre, jamais satisfait, qui passe de l'enthousiasme à la colère en une phrase.

**« CUT! »** Une fois par prise, entre 20 et 40 secondes après le clap :

1. Arrêt sur image : ennemis et joueur figés, son étouffé, bandes noires de cinéma.
2. Le réalisateur commente ce qu'il vient de voir, puis énonce la directive.
3. La directive s'affiche en grand, puis se range en haut de l'écran.
4. « ACTION! » : le combat reprend. L'arrêt dure 4 secondes au plus.

**Directives.** Une seule est active à la fois, jusqu'à la fin de la prise.

| Directive | Réplique | Règle | Univers |
| --- | --- | --- | --- |
| Drame | « Plus de drame ! » | Bonus pour chaque élimination faite sous 30 % de vie | Tous |
| Mouvement | « Ne t'arrête jamais ! » | Bonus si le joueur est en mouvement au moment de l'élimination | Tous |
| Gros plan | « Vise la tête, c'est plus beau ! » | Bonus pour les tirs à la tête uniquement | Tous |
| Cascade | « Je veux des cascades ! » | Bonus pour une élimination en glissade ou en l'air | Médiéval, film noir |
| Sans filet | « Sans le bouclier ! » | Le bouclier est désactivé | Médiéval |
| Corps à corps | « Range cet arc ! » | L'arc est désactivé | Médiéval |
| Plan-séquence | « Pas de coupure ! » | Le rechargement est désactivé : le chargeur se remplit à chaque élimination | Film noir |
| Économie | « Le laser coûte cher ! » | La surchauffe arrive deux fois plus vite | Spatial |

**Audimat.** Le score va de 0 à 1 000 et s'affiche comme une jauge d'audience. Valeurs de départ :

| Événement | Effet |
| --- | --- |
| Élimination | + 10 |
| Élimination qui respecte la directive | + 25 |
| Élimination alors que la directive est ignorée | + 5 |
| Prise terminée sans mourir | + 40 |
| Mort du joueur | − 60 |

Les directives qui désactivent quelque chose (bouclier, arc, rechargement) n'ont pas de version « ignorée » : chaque élimination vaut + 25 tant qu'elles sont actives.

**Étoiles.** À l'écran de fin : 1 étoile en dessous de 200, 2 à partir de 200, 3 à partir de 400, 4 à partir de 600, 5 à partir de 800. Les seuils seront recalés samedi sur une partie réelle : une partie correcte doit donner 3 étoiles.

## Level design

Tout le jeu se passe sur un seul plateau de tournage, rhabillé trois fois : la géométrie de base ne change pas, seuls les décors posés dessus changent.

**Le plateau**

- Un hangar de studio d'environ 50 m sur 40 m, visible derrière les décors : murs nus, passerelles, projecteurs, câbles. Il n'existe que pendant la répétition : à la transition, il disparaît et l'univers s'étend jusqu'à l'horizon.
- Au centre, une zone de jeu avec deux niveaux de hauteur reliés par des rampes. Les rampes sont longues, pour que la glissade serve.
- Quatre portes de plateau par où entrent les figurants.
- Des obstacles à mi-hauteur, franchissables d'un saut, qui servent de couverts.
- Un espace central dégagé en hauteur, pour le vol en apesanteur et les drones.

**Les trois habillages**

|  | Médiéval | Spatial | Film noir |
| --- | --- | --- | --- |
| Décor | Cour de château en panneaux peints, créneaux en carton | Hangar de station, coursives, hublots sur fond étoilé imprimé | Ruelle, façades, lampadaires, voiture d'époque |
| Lumière | Chaude, soleil de studio | Froide, néons bleus | Très contrastée, puis noir et blanc au clap |
| Couverts | Tonneaux, charrettes | Caisses, consoles | Voitures, poubelles, kiosques |
| Événement | Un convoi de chevaux en bois, poussé sur rails par des techniciens, traverse le plateau et renverse tout sur son passage | La gravité artificielle se rallume 10 secondes : joueur et drones retombent au sol | Coupure de courant : le plateau est noir, seuls les tirs éclairent |

Chaque événement se déclenche une fois par scène, annoncé par le réalisateur trois secondes avant.

**Deux états par univers.** Chaque habillage existe en version carton (première prise) et en version réaliste (la suite). Le studio lui-même (hangar, projecteurs, caméras, techniciens) n'existe que dans la version carton : après la transition, le joueur est plongé dans l'univers, ciel et horizon compris, sans aucune trace du tournage. Les deux versions d'un objet ont la même taille et la même place. La collision n'est pas sur les objets de décor : elle est portée par un blockout gris invisible, commun aux deux états, ce qui garantit que le terrain ne change pas pendant la transition.

**Transition « mise en scène ».** Un rideau de lumière balaie le plateau d'un bout à l'autre en 2 à 3 secondes : devant lui le carton, derrière lui le réaliste. Sur la ligne de passage, une lisière lumineuse et des étincelles, avec un son de pellicule qui s'emballe. Les deux versions sont posées l'une sur l'autre ; un matériau maître commun (`M_SetDressing`) affiche l'une ou l'autre selon la position du rideau.

**Transitions.** Entre deux scènes, les panneaux de décor s'écartent ou descendent des cintres, les lumières changent, des techniciens traversent le plateau. Le joueur reste libre de bouger. Pendant le final, le changement est instantané, sur un flash de clap.

**Le casting.** Une petite zone en fond vert, séparée du plateau : un couloir avec une rampe pour apprendre la glissade et trois cibles en carton pour apprendre le tir.

## Direction artistique

Le jeu montre un film en train de se faire : chaque scène commence en décor de carton, puis devient un vrai film quand le réalisateur améliore la mise en scène. Ce contraste est la signature visuelle du jeu.

- **Pendant la répétition, l'envers du décor est visible.** Dos des panneaux en bois brut, étais, projecteurs sur pied, perche micro, chaises de réalisateur, câbles au sol.
- **Style des modèles.** Deux styles. Version carton : carton, contreplaqué, scotch et peinture à plat, drôle et visiblement faux. Version réaliste : crédible et soignée, matériaux physiques. Chaque style a sa consigne commune, collée dans tous les prompts (voir le guide des assets).
- **Les figurants ont l'air de figurants.** Costumes approximatifs, même mannequin de base, un accessoire qui identifie l'univers (casque, antenne, chapeau).
- **Ton de jeu télévisé.** Typographie grasse, couleurs saturées pour l'interface, incrustations façon régie (compteur de prise, voyant REC).
- **Signature visuelle par univers.** Médiéval : tons chauds. Spatial : bleu et néon. Film noir : noir et blanc avec grain, seul l'audimat reste en couleur.
- **Effets de tournage.** Bandes noires et arrêt sur image au « CUT! », flash blanc au clap, grain de pellicule léger en permanence.

Pour le prix Render Royalty, l'effort du dimanche porte sur l'éclairage du plateau et sur le post-traitement, pas sur de nouveaux modèles.

## Audio

La voix du réalisateur est l'élément sonore principal : elle porte le thème, l'humour et les règles du jeu.

**Voix off.** Une seule voix, générée à l'avance et livrée dans le build, toujours sous-titrée. Langue à décider ; l'anglais est recommandé, les juges étant anglophones.

| Famille de répliques | Nombre visé | Exemple |
| --- | --- | --- |
| Menu et casting | 4 | « Lui ? Bon… on fera avec. » |
| Début de scène | 3 | « Scène un : le Moyen Âge. Des épées, du sang, de l'émotion ! » |
| Clap de prise | 4 | « Prise deux. Et cette fois, avec conviction. » |
| CUT et directives | 8 | « CUT! Trop mou. Sans le bouclier ! » |
| Réactions positives | 5 | « Ça, c'est du cinéma ! » |
| Réactions négatives | 5 | « J'avais dit la tête. » |
| Mort du joueur | 4 | « Coupez. Il est encore mort. On la refait. » |
| Événements | 3 | « Envoyez les chevaux ! » |
| Final | 4 | « Je n'arrive pas à choisir. On garde tout ! » |
| Écran de fin, par palier d'étoiles | 5 | « Un navet. Mais c'est MON navet. » |

Une réplique ne coupe jamais une autre : la nouvelle attend, ou est abandonnée si elle n'est plus d'actualité.

**Musique.** Une boucle par univers (fanfare médiévale, synthé spatial, jazz), une pour le menu. Pendant le final, la musique change avec l'univers. Au « CUT! », elle s'arrête net.

**Sons.** Clap, sonnerie de plateau, tirs et impacts par arme, mort des figurants, public de studio (applaudissements quand l'audimat monte, murmures quand il baisse), bruits de coulisses pendant les transitions.

Source des sons et musiques : WeLoveIndies, usage non commercial.

## Interface

L'interface imite une régie de tournage et ne montre que ce qui sert à jouer.

**Menu principal.** Un clap géant sur le plateau vide, avec le titre RESHOOT. Trois boutons : ACTION (jouer), Réglages (volume, sensibilité de la souris), Quitter. Cliquer sur ACTION fait claquer le clap.

**En jeu**

| Élément | Position | Contenu |
| --- | --- | --- |
| Vie | Bas gauche | Barre de 0 à 100 |
| Arme | Bas droite | Icône, munitions ou jauge de surchauffe |
| Audimat | Haut droite | Jauge d'audience de 0 à 1 000, qui pulse à chaque gain |
| Directive | Haut centre | Titre de la directive active, vide s'il n'y en a pas |
| Prise | Haut gauche | « Scène 2, prise 3 » et un voyant REC |
| Sous-titres | Bas centre | Réplique en cours du réalisateur |
| Réticule | Centre | Un point ; il change de forme selon l'arme |

Retours visuels : un marqueur au centre quand un tir touche, un « + 25 » qui s'envole vers la jauge quand la directive est respectée, un bord d'écran rouge quand le joueur est touché.

**Casting.** Une fiche d'acteur s'affiche à côté du personnage (nom, rôle : « premier rôle, faute de mieux »). Les touches apparaissent une par une à l'écran au moment où le joueur en a besoin.

**Écran de fin.** Une affiche de film : titre composé d'un mot par univers tiré d'une liste, le joueur en tête d'affiche, 1 à 5 étoiles, une phrase de critique par palier, puis le détail (éliminations, directives respectées, prises ratées). Deux boutons : Rejouer, Quitter.

**Pause.** Échap ouvre un menu minimal : Reprendre, Réglages, Quitter.

## Périmètre

Le jeu livré lundi contient au minimum une scène complète avec le réalisateur ; tout le reste s'ajoute par ordre de priorité.

| Priorité | Contenu | Échéance |
| --- | --- | --- |
| Indispensable | Déplacement et tir, un univers jouable, vagues, « CUT! », voix off sous-titrée, mort qui relance la prise | Vendredi soir |
| Indispensable | Les trois univers, transition carton vers réaliste, directives, audimat, final, menu, écran de fin | Samedi soir |
| Important | Casting-tutoriel, événements d'univers, transitions de décor à vue, public de studio | Samedi soir |
| Bonus | Escalade des rebords, mitraillette d'époque, second type d'ennemi en spatial et en film noir, DLSS | Dimanche, seulement si tout le reste est fini |

Exclu du jeu, quoi qu'il arrive :

- Multijoueur, en ligne ou local.
- Appels à une IA pendant la partie.
- Inventaire, améliorations, progression entre les parties, sauvegarde.
- Capture de mouvement et cinématiques.
- Version mobile ou manette.

Les documents techniques sont dans le dépôt : `AGENTS.md` (règles pour les IA), `docs/ARCHITECTURE.md` (classes et interfaces), `docs/TASKS.md` (tâches par jour), `docs/DEVLOG.md` (journal). Le planning et la stratégie de prix sont dans le brief d'équipe.
