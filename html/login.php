<?php
session_start();
echo "Contenu de php://input : " . file_get_contents("php://input");
var_dump($_POST);
var_dump(file_get_contents("php://input"));
// Simuler des utilisateurs pour l'authentification
$users = [
    'admin' => '1234',
    'user1' => 'user1pass',
    'user2' => 'user2pass'
];

$error = ""; // Initialiser $error comme une chaîne vide

// Vérifier si la méthode de la requête est POST
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    // Vérifier si les champs 'username' et 'password' existent dans $_POST
    $username = isset($_POST['username']) ? $_POST['username'] : null;
    $password = isset($_POST['password']) ? $_POST['password'] : null;

    // Vérifier que les deux champs ont été fournis
    if ($username !== null && $password !== null) {
        // Vérifier si le nom d'utilisateur existe et que le mot de passe correspond
        if (isset($users[$username]) && $users[$username] === $password) {
            $_SESSION['username'] = $username;
            // Rediriger vers une autre page en cas de succès
            header("Location: welcome.php");
            exit();
        } else {
            // Si la connexion échoue, affiche un message d'erreur
            $error = "Nom d'utilisateur ou mot de passe incorrect.";
        }
    } else {
        $error = "Veuillez remplir tous les champs.";
    }
}
?>

<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Connexion</title>
</head>
<body>
    <h2>Connexion Utilisateur</h2>
    <?php
    if (!empty($error)) {
        echo "<p style='color: red;'>$error</p>";
    }
    ?>
    <form action="login.php" method="post">
        <label for="username">Nom d'utilisateur :</label>
        <input type="text" id="username" name="username" required><br><br>
        <label for="password">Mot de passe :</label>
        <input type="password" id="password" name="password" required><br><br>
        <input type="submit" value="Se connecter">
    </form>
</body>
</html>
