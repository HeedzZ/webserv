#!/usr/bin/env python3
import sys
import os
import urllib.parse

# Lire le body depuis l'entrée standard
body = sys.stdin.read()

# Analyser les données du formulaire
params = urllib.parse.parse_qs(body)

# Vérifier les identifiants de connexion
username = params.get('username', [''])[0]
password = params.get('password', [''])[0]

# Identifiants de connexion valides (à remplacer par un système sécurisé dans un contexte réel)
VALID_USERNAME = "admin"
VALID_PASSWORD = "password123"

if username == VALID_USERNAME and password == VALID_PASSWORD:
    # Redirection en cas de succès
    print("Status: 302 Found")
    print("Location: http://localhost:8081/welcome.html")  # Remplacer par l'URL de votre page de succès
    print()
else:
    print("Status: 302 Found")
    print("Location: http://localhost:8081/login.html")  # Remplacer par l'URL de votre page de succès
    print()
