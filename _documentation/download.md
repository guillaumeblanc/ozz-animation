---
title: Downloads
layout: full
keywords: download,sources,binary,git,clone,build,package,gcc,clang,msvc,visual studio,mac,osx,ios,windows,linux,debian,freebsd
order: 20
---

{% include links.jekyll %}

The current version is {{site.data.versions[0].version}}. See [latest CHANGES][link_latest_changes] file for all the details.

Sources
=======

ozz-animation is hosted on [github][link_github]. The latest versions of the source code, including all release tags and incoming branches, are available from there. Get a local copy of the ozz-animation git repository with this command:

{% highlight bash %}
git clone https://github.com/guillaumeblanc/ozz-animation.git
{% endhighlight %}

Alternatively, latest release sources can be downloaded as [packages][link_latest_sources].

Binaries
========

Pre-compiled binaries (libraries, samples...) of the latest release for all supported platforms can be downloaded from [github release][link_latest_release] page.

Dashboard
=========

Use the continuous integration build [dashboard][link_dashboard] to verify branches stability.

The following table automatically reflects all existing github branches.

{% include table.jekyll id="dashboard_table" data=site.data.dashboard %}

<script>
var xmlhttp = new XMLHttpRequest();
xmlhttp.onreadystatechange = function() {
  if (this.readyState == XMLHttpRequest.DONE) {
    if (this.status == 200) {
      populate_table(JSON.parse(this.responseText));
    } else {
      document.getElementById("dashboard_table").innerHTML += "Failed to list branches.";
    }
  }
};
xmlhttp.open("GET", "http://api.github.com/repos/guillaumeblanc/ozz-animation/branches", true);
xmlhttp.send();

function populate_row(branch) {
  var row = "";
  row += '<tr>';
  row += '<th>' + branch.name + '</th>';
  row += '<th><img src="https://travis-ci.org/guillaumeblanc/ozz-animation.svg?branch=' + branch.name + '" alt="' + branch.name + '"></th>';
  row += '<th><img src="https://travis-ci.org/guillaumeblanc/ozz-animation.svg?branch=' + branch.name + '" alt="' + branch.name + '"></th>';
  row += '<th><img src="https://ci.appveyor.com/api/projects/status/github/guillaumeblanc/ozz-animation?branch=' + branch.name + '&svg=true"></th>';
  row += '</tr>';
  return row;
}

function populate_table(branches) {
    branches.sort(function(a, b){
    	if(a.name == "master") return -1;
    	if(b.name == "master") return 1;
    	if(a.name == "develop") return -1;
    	if(b.name == "develop") return 1;
    	if(a.name.includes("hotfix") && !b.name.includes("hotfix")) return -1;
    	if(b.name.includes("hotfix") && !a.name.includes("hotfix")) return 1;
    	if(a.name.includes("release") && !b.name.includes("release")) return -1;
    	if(b.name.includes("release") && !a.name.includes("release")) return 1;
    	if(a.name.includes("feature") && !b.name.includes("feature")) return -1;
    	if(b.name.includes("feature") && !a.name.includes("feature")) return 1;
    	else return a.name.localeCompare(b.name);
    });

    var out = "";
    for(var i = 0; i < branches.length; i++) {
    	var branch = branches[i];
    	if (branch.name.includes("gh-pages")) {
    		continue;
    	}
    	out += populate_row(branch)

    }
    document.getElementById("dashboard_table").innerHTML += out;
}
</script>