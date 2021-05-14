Le programme marche en tant que Mini SHELL et répond bien aux besoins de toutes les questions sauf la question Bonus (Question 8),
Le programme récupére la commande tappée par l'utilisateur et applique strsep dessus, la liste des séparateurs est la suivante : 
						,-!-?-%-$-£-^-(-)-[-]-;-\-_-TAB 
On quitte le mini Shell en tapant exit ou CTRL+D
On peut se déplacer dans les différents répertoires avec la commande "cd"
On peut éxecuter les commandes simples du type : ls -a, on peut aussi éxecuter les commandes avec des redirections du type : ls -a > out 2> err, On peut aussi éxecuter les commandes qui prennent des entrées du type : sort -r < out 
On peut aussi lancer les commandes avec les pipes, cela fonctionne de la manière suivante:
	-On parse la commande, on fait en sorte qu'il y ait autant de processus que de commandes, par exemple : ls | grep m, on aura deux 	processus, le premier effectuera la première commande et enverra le résultat au deuxième, le deuxième aura le stdout du premier 		processus comme stdin, et il va éxecuter la commande grep m
Il n'y a pas de limites de pipes, exemple de commandes : ls | grep m | grep e | grep s | wc -l 
On peut aussi éxecuter les commandes que l'on souhaite lancer en tâche de fond tel : sleep(5) &, pour ce genre de commandes, le père n'attend pas le fils qui éxecute la commande
Le dossier contient un makefile qui éxecute le fichier "miniProjet.c" et crée un éxecutable nommé "mishell"

