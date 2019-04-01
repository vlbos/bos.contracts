personal.bos
-----------

This bos contract allows users to maintain personal data
actions:
void setpersonal(name account,string key,string value);
void sethomepage(name account,string url);

it's a simple key-value map.
sethomepage actually calls setpersonal, just set key to "homepage".
by supplying homepage, dapp can guide users to their homepage.


