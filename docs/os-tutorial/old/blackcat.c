// based on nyancat of klange. modified by oildum.

#include <stdio.h>
#include <stddef.h>

/*
 * Pop Tart Cat animation frames
 */

const char * frame0[] = {
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",.,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,''''''''''''''',,,,,,,,,,,,,,,,,,,,,,,,",
",,>>>>>>>>,,,,,,,,>>>>>>'@@@@@@@@@@@@@@@',,,,,,,,,,,,,,,,,,,,,,,",
">>>>>>>>>>>>>>>>>>>>>>>'@@@$$$$$$$$$$$@@@',,,,,,,,,,,,,,,,,,,,,,",
">>&&&&&&&&>>>>>>>>&&&&&'@@$$$$$-$$-$$$$@@',,,,,,,,,,,,,,,,,,,,,,",
"&&&&&&&&&&&&&&&&&&&&&&&'@$$-$$$$$$''$-$$@','',,,,,,,,,,,,,,,,,,,",
"&&&&&&&&&&&&&&&&&&&&&&&'@$$$$$$$$'**'$$$@''**',,,,,,,,,,,,,,,,,,",
"&&++++++++&&&&&&&&'''++'@$$$$$-$$'***$$$@'***',,,,,,,,,,,,,,,,,,",
"++++++++++++++++++**''+'@$$$$$$$$'***''''****',,,,,,,,,,,,,,,,,,",
"++++++++++++++++++'**'''@$$$$$$$$'***********',,,,,,,,,,,,,,,,,,",
"++########++++++++''**''@$$$$$$-'*************',,,,,,,,,,,,,,,,,",
"###################''**'@$-$$$$$'***.'****.'**',,,,,,,,,,,,,,,,,",
"####################''''@$$$$$$$'***''**'*''**',,,,,,,,,,,,,,,,,",
"##========########====''@@$$$-$$'*%%********%%',,,,,,,,,,,,,,,,,",
"======================='@@@$$$$$$'***''''''**',,,,,,,,,,,,,,,,,,",
"==;;;;;;;;.=======;;;;'''@@@@@@@@@'*********',,,,,,,,,,,,,,,,,,,",
";;;;;;;;;;;;;;;;;;;;;'***''''''''''''''''''',,,,,,,,,,,,,,,,,,,,",
";;;;;;;;;;;;;;;;;;;;;'**'','*',,,,,'*','**',,,,,,,,,,,,,,,,,,,,,",
";;,,,,,.,,;;;.;;;;,,,'''',,'',,,,,,,'',,'',,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,.,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
};

const char * frame1[] = {
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,''''''''''''''',,,,,,,,,,,,,,,,,,,,,,,,",
",,>>>>>>>>,,,,,,,,>>>>>>'@@@@@@@@@@@@@@@',,,,,,,,,,,,,,,,,,,,,,,",
">>>>>>>>>>>>>>>>>>>>>>>'@@@$$$$$$$$$$$@@@',,,,,,,,,,,,,,,,,,,,,,",
">>&&&&&&&&>>>>>>>>&&&&&'@@$$$$$-$$-$$$$@@',,,,,,,,,,,,,,,,,,,,,,",
"&&&&&&&&&&&&&&&&&&&&&&&'@$$-$$$$$$$''-$$@',,'',,,,,,,,,,,,,,,,,,",
"&&&&&&&&&&&&&&&&&&&&&&&'@$$$$$$$$$'**'$$@','**',,,,,,,,,,,,,,,,,",
"&&++++++++&&&&&&&&+++++'@$$$$$-$$$'***$$@''***',,,,,,,,,,,,,,,,,",
"+++++++++++++++++++'+++'@$$$$$$$$$'***''''****',,,,,,,,,,,,,,,,,",
"++++++++++++++++++'*'++'@$$$$$$$$$'***********',,,,,,,,,,,,,,,,,",
"++########++++++++'*''''@$$$$$$-$'*************',,,,,,,,,,,,,,,,",
"###################****'@$-$$$$$$'***.'****.'**',,,,,,,,,,,,,,,,",
"###################''**'@$$$$$$$$'***''**'*''**',,,,,,,,,,,,,,,,",
"##========########==='''@@$$$-$$$'*%%********%%',,,,,,,,,,,,,,,,",
"======================='@@@$$$$$$$'***''''''**',,,,,,,,,,,,,,,,,",
"==;;;;;;;;========;;;;;''@@@@@@@@@@'*********',,,,,,,,,,,,,,,,,,",
";;;;;;;;;;;;;;;;;;;;;;'**'''''''''''''''''''',,,,,,,,,,,,,,,,,,,",
";;;;;;;;;;;;;;;;;;;;;;'**','*',,,,,,**','**',,,,,,,,,,,,,,,,,,,,",
";;,,,.,,,,;;;;;;;;,,,,''',,,'',,,,,,''',,''',,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
};

const char * frame2[] = {
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,.",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,.",
">>,,,,,,,>>>>>>>>,,,,,,,,''''''''''''''',,,,,,,,,,,,,,,,,,,,,,,,",
">>>>>>>>>>>>>>>>>>>>>>>>'@@@@@@@@@@@@@@@',,,,,,,,,,,,,,,,,,,,,,,",
"&&>>>>>>>&&&&&&&&>>>>>>'@@@$$$$$$$$$$$@@@',,,,,,,,,,,,,,,,,,,,,,",
"&&&&&&&&&&&&&&&&&&&&&&&'@@$$$$$-$$-$$$$@@',,,,,,,,,,,,,,,,,,,,,,",
"&&&&&&&&&&&&&&&&&&&&&&&'@$$-$$$$$$$''-$$@',,'',,,,,,,,,,,,,,,,,,",
"++&&&&&&&++++++++&&&&&&'@$$$$$$$$$'**'$$@','**',,,,,,,,,,,,,,,,,",
"+++++++++++++++++++++++'@$$$$$-$$$'***$$@''***',,,,,,,,,,,,,,,,,",
"+++++++++++++++++++++++'@$$$$$$$$$'***''''****',,,,,,,,,,,,,,,,,",
"##+++++++########++++++'@$$$$$$$$$'***********',,,,,,,,,,,,,,,,,",
"######################''@$$$$$$-$'*************',,,,,,,,,,,,,,,,",
"###################'''''@$-$$$$$$'***.'****.'**',,,,,,,,,,,,,,,,",
"==#######========#'****'@$$$$$$$$'***''**'*''**',,,,,,,,,,,,,,,,",
"==================='''='@@$$$-$$$'*%%********%%',,,,,,,,,,,,,,,,",
";;=======;;;;;;;;======'@@@$$$$$$$'***''''''**',,,,,,,,,,,,,,,,,",
";;;;;;;;;;;;;;;;;;;;;;;''@@@@@@@@@@'*********',,,,,,,,,,,,,,,,,,",
";.;;;;;;;;;;;;;;;;;;;;;'*'''''''''''''''''''',,,,,,,,,,,,,,,,,,,",
".,.;;;;;;,,,,,,,,;;;;;;'**',**',,,,,,**','**',,,,,,,,,,,,,,,,,,,",
",.,,,,,,,,,,,,,,,,,,,,,''',,''',,,,,,''',,''',,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
};

const char * frame3[] = {
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,.,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,.,,,.,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,.,,,,,,,",
">>,,,,,,,>>>>>>>>,,,,,,,,''''''''''''''',,,,,,,,,,,,,,,,,,,,,,,,",
">>>>>>>>>>>>>>>>>>>>>>>>'@@@@@@@@@@@@@@@',,,,,,,,,,,,,,,,,,,,,,,",
"&&>>>>>>>&&&&&&&&>>>>>>'@@@$$$$$$$$$$$@@@',,,,,,,,,,,,,,,,,,,,,,",
"&&&&&&&&&&&&&&&&&&&&&&&'@@$$$$$-$$-$$$$@@',,,,,,,,,,,,,,,,,,,,,,",
"&&&&&&&&&&&&&&&&&&&&&&&'@$$-$$$$$$$''-$$@',,'',,,,,,,,,,,,,,,,,,",
"++&&&&&&&++++++++&&&&&&'@$$$$$$$$$'**'$$@','**',,,,,,,,,,,,,,,,,",
"+++++++++++++++++++++++'@$$$$$-$$$'***$$@''***',,,,,,,,,,,,,,,,,",
"+++++++++++++++++++++++'@$$$$$$$$$'***''''****',,,,,,,,,,,,,,,,,",
"##+++++++########++++++'@$$$$$$$$$'***********',,,,,,,,,,,,,,,,,",
"#####################'''@$$$$$$-$'*************',,,,,,,,,,,,,,,,",
"###################''**'@$-$$$$$$'***.'****.'**',,,,,,,,,,,,,,,,",
"==#######========##****'@$$$$$$$$'***''**'*''**',,,,,,,,,,,,,,,,",
"=================='*'=='@@$$$-$$$'*%%********%%',,,,,,,,,,,,,,,,",
";;=======;;;;;;;;=='==='@@@$$$$$$$'***''''''**',,,,,,,,,,,,,,,,,",
";;;;;;;;;;;;;;;;;;;;;;;''@@@@@@@@@@'*********',,,,,,,,,,,,,,,,,,",
";;;;;;;;;;;;;;;;;;;;;;'**'''''''''''''''''''',,,,,,,,,,,,,,,,,,,",
",,;;;;;;;,,,,,,,,;;;;;'**','*',,,,,,'*','**',,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,''',,,'',,,,,,,'',,''',,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,.,,,,,,,",
};

const char * frame4[] = {
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,.,,,,,,,,,,,,,,",
",,>>>>>>>>,,,,,,,,>>>>>>>''''''''''''''',,,,,,,,,,,,,,,,,,,,,,,,",
">>>>>>>>>>>>>>>>>>>>>>>>'@@@@@@@@@@@@@@@',,,,,,,,,,,,,,,,,,,,,,,",
">>&&&&&&&&>>>>>>>>&&&&&'@@@$$$$$$$$$$$@@@',,,,,,,,,,,,,,,,,,,,,,",
"&&&&&&&&&&&&&&&&&&&&&&&'@@$$$$$-$$-$$$$@@',,,,,,,,,,,,,,,,,,,,,,",
"&&&&&&&&&&&&&&&&&&&&&&&'@$$-$$$$$$''$-$$@','',,,,,,,,,,,,,,,,,,,",
"&&++++++++&&&&&&&&+++++'@$$$$$$$$'**'$$$@''**',,,,,,,,,,,,,,,,,,",
"+++++++++++++++++++++++'@$$$$$-$$'***$$$@'***',,,,,,,,,,,,,,,,,,",
"++++++++++++++++++'''++'@$$$$$$$$'***''''****',,,,,,,,,,,,,,,,,,",
"++########+++++++'**''''@$$$$$$$$'***********',,,,,,,,,,,,,,,,,,",
"#################'****''@$$$$$$-'*************',,,,,,,,,,,,,,,,,",
"##################''''*'@$-$$$$$'***.'****.'**',,,,,,,,,,,,,,,,,",
"##========########==='''@$$$$$$$'***''**'*''**',,,,,,,,,,,,,,,,,",
"======================='@@$$$-$$'*%%********%%',,,,,,,,,,,,,,,,,",
"==;;;;;;;;========;;;;''@@@$$$$$$'***''''''**',,,,,,,,,,,,,,,,,,",
";;;;;;;;;;;;;;;;;;;;;''''@@@@@@@@@'*********',,,,,,,,,,,,,,,,,,,",
";;;;;;;;;;;;;;;;;;;;'***'''''''''''''''''''',,,,,,,,,,,,,,,,,,,,",
";;,,,,,,,,;;;;;;;;,,'**','**,,,,,,'**,'**',,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,''',,,'',,,,,,,'',,''',,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
};

const char * frame5[] = {
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,>>>>>>>>,,,,,,,,>>>>>>>''''''''''''''',,,,,,,,,,,,,,,,,,,,,,,,",
">>>>>>>>>>>>>>>>>>>>>>>>'@@@@@@@@@@@@@@@',,,,,,,,,,,,,,,,,,,,,,,",
">>&&&&&&&&>>>>>>>>&&&&&'@@@$$$$$$$$$$$@@@',,,,,,,,,,,,,,,,,,,,,,",
"&&&&&&&&&&&&&&&&&&&&&&&'@@$$$$$-$$''$$$@@','',,,,,,,,,,,,,,,,,,,",
"&&&&&&&&&&&&&&&&&&&&&&&'@$$-$$$$$'**'-$$@''**',,,,,,,,,,,,,,,,,,",
"&&++++++++&&&&&&&&+++++'@$$$$$$$$'***$$$@'***',,,,,,,,,,,,,,,,,,",
"+++++++++++++++++++'+++'@$$$$$-$$'***''''****',,,,,,,,,,,,,,,,,,",
"++++++++++++++++++'*'++'@$$$$$$$$'***********',,,,,,,,,,,,,,,,,,",
"++########++++++++'*''''@$$$$$$$'*************',,,,,,,,,,,,,,,,,",
"###################****'@$$$$$$-'***.'****.'**',,,,,,,,,,,,,,,,,",
"###################''**'@$-$$$$$'***''**'*''**',,,,,,,,,,,,,,,,,",
"##========########==='''@$$$$$$$'*%%********%%',,,,,,,,,,,,,,,,,",
"======================='@@$$$-$$$'***''''''**',,,,,,,,,,,,,,,,,,",
"==;;;;;;;;========;;;;''@@@$$$$$$$'*********',,,,,,,,,,,,,,,,,,.",
";;;;;;;;;;;;;;;;;;;;;'*''@@@@@@@@@@''''''''',,,,,,,,,,,,,,,,,,,.",
";;;;;;;;;;;;;;;;;;;;'***''''''''''''''''*',,,,,,,,,,,,,,,,,,,,,,",
";;,,,,,,,,;;;;;;;;,,'**','**,,,,,,'**,'**',,,,,,,,,,,,,,,,,,..,.",
",,,,,,,,,,,,,,,,,,,,''',,''',,,,,,''',,''',,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,.",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,.",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
};

const char * frame6[] = {
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,.,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,''''''''''''''',,,,,,,,,,,,,,,,,,,,,,,,",
">>,,,,,,,>>>>>>>>,,,,,,,'@@@@@@@@@@@@@@@',,,,,,,,,,,,,,,,,,,,,,,",
">>>>>>>>>>>>>>>>>>>>>>>'@@@$$$$$$$$$$$@@@',,,,,,,,,,,,,,,,,,,,,,",
"&&>>>>>>>&&&&&&&&>>>>>>'@@$$$$$-$$-$$$$@@',,,,,,,,,,,,,,,,,,,,,,",
"&&&&&&&&&&&&&&&&&&&&&&&'@$$-$$$$$$''$-$$@','',,,,,,,,,,,,,,,,,,,",
"&&&&&&&&&&&&&&&&&&&&&&&'@$$$$$$$$'**'$$$@''**',,,,,,,,,,,,,,,,,,",
"++&&&&&&&++++++++&'''&&'@$$$$$-$$'***$$$@'***',,,,,,,,,,,,,,,,,,",
"++++++++++++++++++'*''+'@$$$$$$$$'***''''****',,,,,,,,,,,,,,,,,,",
"++++++++++++++++++'**'''@$$$$$$$$'***********',,,,,,,,,,,,,,,,,,",
"##+++++++########++'**''@$$$$$$-'*************',,,,,,,,,,,,,,,,,",
"###################''**'@$-$$$$$'***.'****.'**',,,,,,,,,,,,,,,,,",
"####################''''@$$$$$$$'***''**'*''**',,,,,,,,,,,,,,,,,",
"==#######========#####''@@$$$-$$'*%%********%%',,,,,,,,,,,,,,,,,",
"======================='@@@$$$$$$'***''''''**',,,,,,,,,,,,,,,,,,",
";;=======;;;;;;;;====='''@@@@@@@@@'*********',,,,,,,,,,,.,,,,,,,",
";;;;;;;;;;;;;;;;;;;;;'***''''''''''''''''''',,,,,,,,,,.,,,.,,,,,",
";;;;;;;;;;;;;;;;;;;;;'**'','*',,,,,'**,'**',,,,,,,,,,,,,,,,,,,,,",
",,;;;;;;;,,,,,,,,;;;;'''',,'',,,,,,,'',,'',,,,,,,,,,,.,,,,,.,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,.,,,.,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
};

const char * frame7[] = {
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,..,..,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,.,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,.,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,''''''''''''''',,,,,,,,,,,,,,,,,,,,,,,,",
">>,,,,,,,>>>>>>>>,,,,,,,'@@@@@@@@@@@@@@@',,,,,,,,,,,,,,,,,,,,,,,",
">>>>>>>>>>>>>>>>>>>>>>>'@@@$$$$$$$$$$$@@@',,,,,,,,,,,,,,,,,,,,,,",
"&&>>>>>>>&&&&&&&&>>>>>>'@@$$$$$-$$-$$$$@@',,,,,,,,,,,,,,,,,,,,,,",
"&&&&&&&&&&&&&&&&&&&&&&&'@$$-$$$$$$$''-$$@',,'',,,,,,,,,,,,,,,,,,",
"&&&&&&&&&&&&&&&&&&&&&&&'@$$$$$$$$$'**'$$@','**',,,,,,,,,,,,,,,,,",
"++&&&&&&&++++++++&&&&&&'@$$$$$-$$$'***$$@''***',,,,,,,,,,,,,,,,,",
"+++++++++++++++++++'+++'@$$$$$$$$$'***''''****',,,,,,,,,,,,,,,,,",
"++++++++++++++++++'*'++'@$$$$$$$$$'***********',,,,,,,,,,,,,,,,,",
"##+++++++########+'*''''@$$$$$$-$'*************',,,,,,,,,,,,,,,,",
"###################****'@$-$$$$$$'***.'****.'**',,,,,,,,,,,,,,,,",
"###################''**'@$$$$$$$$'***''**'*''**',,,,,,,,,,,,,,,,",
"==#######========####'''@@$$$-$$$'*%%********%%',,,,,,,,,,,,,,,,",
"======================='@@@$$$$$$$'***''''''**',,,,,,,,,,,,,,,,,",
";;=======;;;;;;;;======''@@@@@@@@@@'*********',,,.,,,,,,,,,,,,,,",
";;;;;;;;;;;;;;;;;;;;;;'**'''''''''''''''''''',,,,,,,,,,,,,,,,,,,",
";;;;;;;;;;;;;;;;;;;;;;'**','*',,,,,,**','**',,,,,,,,,,,,,,,,,,,,",
",,;;;;;;;,,,,,,,,;;;;;''',,,'',,,,,,''',,''',,.,,,,.,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
};

const char * frame8[] = {
",,,,,,,,,,,,,,,,,,,,,,,,..,...,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,.,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,.,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,>>>>>>>>,,,,,,,,>>>>>>>''''''''''''''',,,,,,,,,,,,,,,,,,,,,,,,",
">>>>>>>>>>>>>>>>>>>>>>>>'@@@@@@@@@@@@@@@',,,,,,,,,,,,,,,,,,,,,,,",
">>&&&&&&&&>>>>>>>>&&&&&'@@@$$$$$$$$$$$@@@',,,,,,,,,,,,,,,,,,,,,,",
"&&&&&&&&&&&&&&&&&&&&&&&'@@$$$$$-$$-$$$$@@',,,,,,,,,,,,,,,,,,,,,,",
"&&&&&&&&&&&&&&&&&&&&&&&'@$$-$$$$$$$''-$$@',,'',,,,,,,,,,,,,,,,,,",
"&&++++++++&&&&&&&&+++++'@$$$$$$$$$'**'$$@','**',,,,,,,,,,,,,,,,,",
"+++++++++++++++++++++++'@$$$$$-$$$'***$$@''***',,,,,,,,,,,,,,,,,",
"+++++++++++++++++++++++'@$$$$$$$$$'***''''****',,,,,,,,,,,,,,,,,",
"++########++++++++#####'@$$$$$$$$$'***********',,,,,,,,,,,,,,,,,",
"######################''@$$$$$$-$'*************',,,,,,,,,,,,,,,,",
"###################'''''@$-$$$$$$'***.'****.'**',,,,,,,,,,,,,,,,",
"##========########'****'@$$$$$$$$'***''**'*''**',,,,,,,,,,,,,,,,",
"==================='''='@@$$$-$$$'*%%********%%',,,,,,,,,,,,,,,,",
"==;;;;;;;;========;;;;;'@@@$$$$$$$'***''''''**',,,,,,,,,,,,,,,,,",
";;;;;;;;;;;;;;;;;;;;;;;''@@@@@@@@@@'*********',,,,,,,,,,,,,,,,,,",
";;;;;;;;;;;;;;;;;;;;;;;'*'''''''''''''''''''',,,,,,,,,,,,,,,,,,,",
";;,,,,,,,,;;;;;;;;,,,,,'**',**',,,,,,**'.'**',,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,''',,''',,,,,,''',,''',,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
};

const char * frame9[] = {
",,,,,,,,,,,,,,,,,,,,.,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,.,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,>>>>>>>>,,,,,,,,>>>>>>>''''''''''''''',,,,,,,,,,,,,,,,,,,,,,,,",
">>>>>>>>>>>>>>>>>>>>>>>>'@@@@@@@@@@@@@@@',,,,,,,,,,,,,,,,,,,,,,,",
">>&&&&&&&&>>>>>>>>&&&&&'@@@$$$$$$$$$$$@@@',,,,,,,,,,,,,,,,,,,,,,",
"&&&&&&&&&&&&&&&&&&&&&&&'@@$$$$$-$$-$$$$@@',,,,,,,,,,,,,,,,,,,,,,",
"&&&&&&&&&&&&&&&&&&&&&&&'@$$-$$$$$$$''-$$@',,'',,,,,,,,,,,,,,,,,,",
"&&++++++++&&&&&&&&+++++'@$$$$$$$$$'**'$$@','**',,,,,,,,,,,,,,,,,",
"+++++++++++++++++++++++'@$$$$$-$$$'***$$@''***',,,,,,,,,,,,,,,,,",
"+++++++++++++++++++++++'@$$$$$$$$$'***''''****',,,,,,,,,,,,,,,,,",
"++########++++++++#####'@$$$$$$$$$'***********',,,,,,,,,,,,,,,,,",
"#####################'''@$$$$$$-$'*************',,,,,,,,,,,,,,,,",
"###################''**'@$-$$$$$$'***.'****.'**',,,,,,,,,,,,,,,,",
"##========########=****'@$$$$$$$$'***''**'*''**',,,,,,,,,,,,,,,,",
"=================='*'=='@@$$$-$$$'*%%********%%',,,,,,,,,,,,,,,,",
"==;;;;;;;;========;';;;'@@@$$$$$$$'***''''''**',,,,,,,,,,,,,,,,,",
";;;;;;;;;;;;;;;;;;;;;;;''@@@@@@@@@@'*********',,,,,,,,,,,,,,,,,,",
";;;;;;;;;;;;;;;;;;;;;;'**'''''''''''''''''''',,,,,,,,,,,,,,,,,,,",
";;,,,,,,,,;;;;;;;;,,,,'**','*',,..,.**','**',,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,''',,,'',,,,.,''',,''',,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,.,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
};

const char * frame10[] = {
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,.,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
">>,,,,,,,>>>>>>>>,,,,,,,,''''''''''''''',,,,,,,,,,,,,,,,,,,,,,,,",
">>>>>>>>>>>>>>>>>>>>>>>>'@@@@@@@@@@@@@@@',,,,,,,,,,,,,,,,,,,,,,,",
"&&>>>>>>>&&&&&&&&>>>>>>'@@@$$$$$$$$$$$@@@',,,,,,,,,,,,,,,,,,,,,,",
"&&&&&&&&&&&&&&&&&&&&&&&'@@$$$$$-$$-$$$$@@',,,,,,,,,,,,,,,,,,,,,,",
"&&&&&&&&&&&&&&&&&&&&&&&'@$$-$$$$$$''$-$$@','',,,,,,,,,,,,,,,,,,,",
"++&&&&&&&++++++++&&&&&&'@$$$$$$$$'**'$$$@''**',,,,,,,,,,,,,,,,,,",
"+++++++++++++++++++++++'@$$$$$-$$'***$$$@'***',,,,,,,,,,,,,,,,,,",
"++++++++++++++++++'''++'@$$$$$$$$'***''''****',,,,,,,,,,,,,,,,,,",
"##+++++++########'**''''@$$$$$$$$'***********',,,,,,,,,,,,,,,,,,",
"#################'****''@$$$$$$-'*************',,,,,,,,,,,,,,,,,",
"##################''''*'@$-$$$$$'***.'****.'**',,,,,,,,,,,,,,,,,",
"==#######========####'''@$$$$$$$'***''**'*''**',,,,,,,,,,,,,,,,,",
"======================='@@$$$-$$'*%%********%%',,,,,,,,,,,,,,,,,",
";;=======;;;;;;;;=====''@@@$$$$$$'***''''''**',,,,,,,,,,,,,,,,,,",
";;;;;;;;;;;;;;;;;;;;;''''@@@@@@@@@'*********',,,,,,,,,,,,,,,,,,,",
";;;;;;;;;;;;;;;;;;;;'***'''''''''''''''''''',,,,,,,,,,,,,,,,,,,,",
",,;;;;;;;,,,,,,,,;;;'**'.'**..,,,,'**''**',,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,''',,,'',,,,,,,''',''',,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,.,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,.,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
};

const char *frame11[] = {
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
">>,,,,,,,>>>>>>>>,,,,,,,,''''''''''''''',,,,,,,,,,,,,,,,,,,,,,,,",
">>>>>>>>>>>>>>>>>>>>>>>>'@@@@@@@@@@@@@@@',,,,,,,,,,,,,,,,,,,,,,,",
"&&>>>>>>>&&&&&&&&>>>>>>'@@@$$$$$$$$$$$@@@',,,,,,,,,,,,,,,,,,,,,,",
"&&&&&&&&&&&&&&&&&&&&&&&'@@$$$$$-$$''$$$@@','',,,,,,,,,,,,,,,,,,,",
"&&&&&&&&&&&&&&&&&&&&&&&'@$$-$$$$$'**'-$$@''**',,,,,,,,,,,,,,,,,,",
"++&&&&&&&++++++++&&&&&&'@$$$$$$$$'***$$$@'***',,,,,,,,,,,,,,,,,,",
"+++++++++++++++++++'+++'@$$$$$-$$'***''''****',,,,,,,,,,,,,,,,,,",
"++++++++++++++++++'*'++'@$$$$$$$$'***********',,,,,,,,,,,,,,,,,,",
"##+++++++########+'*''''@$$$$$$$'*************',,,,,,,,,,,,,,,,,",
"###################****'@$$$$$$-'***.'****.'**',,,,,,,,,,,,,,,,,",
"###################''**'@$-$$$$$'***''**'*''**',,,,,,,,,,,,,,,,,",
"==#######========####'''@$$$$$$$'*%%********%%',,,,,,,,,,,,,,,,,",
"======================='@@$$$-$$$'***''''''**',,,,,,,,,,,,,,,,,,",
";;=======;;;;;;;;=.===''@@@$$$$$$$'*********',,,,,,,,,,,,,,,,,,,",
";;;;;;;;;;;;;;;;;;;;.'*''@@@@@@@@@@''''''''',,,,,,,,,,,,,,,,,,,,",
";;;;;;;;;;;;;;;;;;;;'***''''''''''''''''*',,,,,,,,,,,,,,,,,,,,,,",
",,;;;;;;;,,,,,,,.;;;'**','**,,,,,,'**''**',,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,''',,''',,,,,,''',,''',,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,.,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,.,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,",
};

const char ** frames[] = {
	frame0,
	frame1,
	frame2,
	frame3,
	frame4,
	frame5,
	frame6,
	frame7,
	frame8,
	frame9,
	frame10,
	frame11,
	NULL
};

#define FRAME_WIDTH  64
#define FRAME_HEIGHT 24

#define RESET "\033[0m"

char *colors[256] = {
    [','] = "\033[44m",      /* Blue background */
    ['.'] = "\033[47m",      /* White stars */
    ['\''] = "\033[40m",       /* Black border */
    ['@']  = "\033[47m",       /* Tan poptart */
    ['$']  = "\033[45m",      /* Pink poptart */
    ['-']  = "\033[41m",      /* Red poptart */
    ['>']  = "\033[41m",      /* Red rainbow */
    ['&']  = "\033[43m",       /* Orange rainbow */
    ['+']  = "\033[43m",      /* Yellow Rainbow */
    ['#']  = "\033[42m",      /* Green rainbow */
    ['=']  = "\033[44m",      /* Light blue rainbow */
    [';']  = "\033[44m",       /* Dark blue rainbow */
    ['*']  = "\033[40m",      /* Gray cat face */
    ['%']  = "\033[45m",      /* Pink cheeks */
};

void display_frame(const char **frame)
{
    for (int i = 0; i < FRAME_HEIGHT; i++) {
        const char *line = frame[i];
        for (int j = 0; j < FRAME_WIDTH; j++) {
            char *bg = colors[line[j]];
            if (strlen(bg) == 5) {
                printf("%s ", bg);
            }
        }
        printf("%s\n", RESET);
    }
}

int main()
{
    printf("\033[3J");
    for (int t = 0; t < 100; t++) {
        for (int i = 0; i < 12; i++) {
            printf("\033[H");
            display_frame(frames[i]);
            for (int j = 0; j < 50000000; j++);
        }
    }
    return 0;
}