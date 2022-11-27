function tab(evt, tabName) {
    var i, tabcontent, tablinks;

    tabcontent = document.getElementsByClassName("tabcontent");
    for (i = 0; i < tabcontent.length; i++) {
        tabcontent[i].style.display = "none";
    }

    // Get all elements with class="tablinks" and remove the class "active"
    tablinks = document.getElementsByClassName("tablinks");
    for (i = 0; i < tablinks.length; i++) {
        tablinks[i].className = tablinks[i].className.replace(" active", "");
    }

    document.getElementById(tabName).style.display = "block";
    evt.currentTarget.className += " active";
}
document.getElementsByClassName('tablinks')[0].click()
document.addEventListener("DOMContentLoaded", function() {
    console.log("DOM loaded")
    for (let el of document.querySelectorAll('.advSect')) el.classList.add('hidden');
    for (let el of document.querySelectorAll('.asb')) el.addEventListener('click', function(e){
        console.log("Click:"+'as-'+e.target.dataset.code)
        document.getElementById('as-'+e.target.dataset.code).classList.toggle('hidden')
    });
})