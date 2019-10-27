# Interpréteur de commandes
*ROMON Emile*

*SAUVAGE Quentin*

## Fonctionnalités
- Séquencement de la ligne de commandes
- Gestion des wildcards
- Changement de répertoire
- Exit et gestion du ctrl-C
- status
- myls
- myps
- Pipelines
- Redirections
- Lancement en background 
- myjobs, myfg, mybg
- Variables d'environnement
- Variables locales
- Utilisation de ces variables

## Bugs
- Il est impossible de lancer le programme depuis le programme lui-même (./mysh -> ./mysh). Cela cause une interruption de la deuxième instance,qui empêche la libération de la mémoire partagée.
- Le chemin *affiché* lors du myls avec l'option -R est affiché à partir de la racine, et non à partir du répertoire de départ.
- Lorsqu'une ligne de commande est executée en background, seul le pid correspondant au processus exécutant la première commande est affiché par la commande interne myjobs. Par conséquent, si cette ligne de commande contient plusieurs commande, myjobs affichera toujours le même pid même si le prossessus correspondant est terminé.
- Toujours en ce qui concerne les jobs, il arrive, dans certaines situations, que l'affichage du statut d'un processus par myjobs ne corresponde pas a la réalité ou que le programme se bloque si l'utilisateur utilise la commande kill pour interagir avec les processus au lieu d'utiliser les commandes internes myfg et mybg.
