#!/usr/bin/env python3

import sqlite3

# Chemin vers la base de données
DATABASE_PATH = 'users.db'

print("Content-Type: text/html\n")

try:
    # Connexion à la base de données
    conn = sqlite3.connect(DATABASE_PATH)
    c = conn.cursor()

    # Exécution de la requête pour récupérer tous les utilisateurs et leurs mots de passe
    c.execute('SELECT username, password_hash FROM users')

    # Affichage des utilisateurs et de leurs mots de passe hachés
    users = c.fetchall()
    if users:
        print("<table border='1'>")
        print("<tr><th>Nom d'utilisateur</th><th>Mot de passe haché</th></tr>")
        for user in users:
            print(f"<tr><td>{user[0]}</td><td>{user[1]}</td></tr>")
        print("</table>")
    else:
        print("<p>Aucun utilisateur enregistré.</p>")

    # Fermeture de la connexion à la base de données
    conn.close()

except Exception as e:
    print(f"<p>Erreur lors de la récupération des utilisateurs : {str(e)}</p>")
