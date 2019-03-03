function setupGA(id) {
    window.dataLayer = window.dataLayer || [];
    function gtag(){dataLayer.push(arguments);}
    gtag('js', new Date());
    gtag('config', id);
}

function dntEnabed() {
    var dnt = navigator.doNotTrack || window.doNotTrack || navigator.msDoNotTrack;
    return dnt == "1" || dnt == "yes";
}

if (!dntEnabed()) {
    document.write(' \
<script src="https://www.googletagmanager.com/gtag/js?id=UA-93472390-2"> \
</script> \
<script> \
    setupGA("UA-93472390-2"); \
</script>')
}
