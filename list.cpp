#include "icecream.h"

#include <string.h>
#include <strings.h>
#include <sstream>
using namespace YAML;

struct BukkitInfo* bukkitversion() {
	vector<char*> urls;
	vector<char*> codes;
	vector<char*> options;
	int r;

	status("Fetching list of Bukkit versions...");

	char* versiontext = fetchurl(BUKKITURL);
	if (!versiontext || !versiontext[0]) die("Could not fetch list of Bukkit versions.\n");
	
	char* line;
	char* url, * code, * option;
	#ifdef __WIN32__
	// Windows doesn't have strtok_r().
	vector<char*> lines;	
	line = strtok(versiontext, "\r\n");
	while (line) {
		lines.push_back(line);
		line = strtok(NULL, "\r\n");
	}
	
	for (int i = 0; i < lines.size(); i++) {
		url = strtok(lines[i], " ");
		code = strtok(NULL, " ");
		option = strtok(NULL, "\r\n");
		
		urls.push_back(url);
		codes.push_back(code);
		options.push_back(option);
	}
	#else
	char* pos;
	line = strtok_r(versiontext, "\r\n", &pos);
	while (line) {
		url = strtok(line, " ");
		code = strtok(NULL, " ");
		option = strtok(NULL, "\r\n");
		
		urls.push_back(url);
		codes.push_back(code);
		options.push_back(option);
		
		line = strtok_r(NULL, "\r\n", &pos);
	}
	#endif
	
	r = showmenu(options);
	
	BukkitInfo* ret = new BukkitInfo();
	ret->url = strdup(urls[r]);
	ret->code = strdup(codes[r]);
	ret->option = strdup(options[r]);
	return ret;
}

void modlist(vector<Mod>* mods, const char* bukkitcode) {
	char* metaurl = (char*)malloc(strlen(METAURL) + strlen(bukkitcode) + 1);
	sprintf(metaurl, "%s%s", METAURL, bukkitcode);
	
	status("Fetching metalist...");
	char* metatext = fetchurl(metaurl);
	//printf("metatext='%s'\n", metatext);
	
	vector<char*> urls;
	vector<char*> names;

	char* line;
	char* url, * name;
	#ifdef __WIN32__
	vector<char*> lines;	
	line = strtok(metatext, "\r\n");
	while (line) {
		lines.push_back(line);
		line = strtok(NULL, "\r\n");
	}
	
	for (int i = 0; i < lines.size(); i++) {
		url = strtok(lines[i], " ");
		name = strtok(NULL, "\r\n");
		
		urls.push_back(url);
		names.push_back(name);
	}
	#else
	char* pos;
	line = strtok_r(metatext, "\r\n", &pos);
	while (line) {
		url = strtok(line, " ");
		name = strtok(NULL, "\r\n");
		
		urls.push_back(url);
		names.push_back(name);
		
		line = strtok_r(NULL, "\r\n", &pos);
	}
	#endif

	list<Mod> modlist;

	for (unsigned int i = 0; i < urls.size(); i++) {
		status("Fetching list: %s", names[i]);
		char* listtext = fetchurl(urls[i]);

		istringstream in(listtext);
		Parser parser(in);
		
		Node doc;
		try {
			while (parser.GetNextDocument(doc)) {
				const Node& modnode = doc["mods"];
				for (unsigned int j = 0; j < modnode.size(); j++) {
					modlist.push_back(Mod(modnode[j]));
				}
			}
		} catch (ParserException e) {
			die("%s\n", e.what());
		}
	}
	
	
	modlist.sort();
	
	int m;
	while ((m = showmenu(modlist)) >= 0) {
		list<Mod>::iterator i = modlist.begin();
		for (int j = 0; j < m; j++) i++;
		Mod mod = *i;
		modlist.remove(mod);
		
		processdeps(mods, &modlist, mod);
		
		mods->push_back(mod);
	}
}

void processdeps(vector<Mod>* mods, list<Mod>* modlist, const Mod& mod) {
	for (unsigned int i = 0; i < mod.depends.size(); i++) {
		const char* dep = mod.depends[i];
		
		int found = 0;
		for (unsigned int j = 0; j < mods->size(); j++) {
			if ((*mods)[j] == dep) {
				found = 1;
				break;
			}
		}
		if (found) continue;
		
		found = 0;
		for (list<Mod>::iterator j = modlist->begin(); j != modlist->end(); ++j) {
			if (*j == dep) {
				Mod depmod = *j;
				modlist->remove(depmod);
				processdeps(mods, modlist, depmod);
				mods->push_back(depmod);
				log("Added as dependency: %s\n", depmod.desc);
				found = 1;
				break;
			}
		}
		if (found == 0) printf("Warning: Unsatisfied dependency '%s'\n", dep);
	}
}



// EOF

