<?php
session_start();
if (!isset($_SESSION['username'])) {
    // Redirige vers la page de connexion si l'utilisateur n'est pas connecté
    header("Location: login.html");
    exit();
}
?>
<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Bienvenue</title>
</head>
<body>
    <h1>Bienvenue, <?php echo htmlspecialchars($_SESSION['username']); ?> !</h1>
    <p>Vous êtes maintenant connecté.</p>
    <a href="logout.php">Se déconnecter</a>
</body>
</html>