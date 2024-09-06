function predict() {
    var inputFeatures = document.getElementById("input-features").value;
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            document.getElementById("prediction-result").innerHTML = this.responseText;
        }
    };
    xhttp.open("POST", "/predict", true);
    xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    xhttp.send("input_features=" + inputFeatures);
}
