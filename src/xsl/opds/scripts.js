document.getElementById('language-filter').addEventListener('change', function() {
	const selectedLang = this.value;
	const currentUrl = new URL(window.location.href);
	if (selectedLang) {
		currentUrl.searchParams.set('lang', selectedLang);
	} else {
		currentUrl.searchParams.delete('lang');
	}
	window.location.href = currentUrl.toString();
});
