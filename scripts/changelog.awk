BEGIN { cl = 0; api = 0; }
/^CHANGELOG.md$/ { cl=1 }
/^include[/]libpldm/ { api=1 }
END { exit !(cl || !api); }
