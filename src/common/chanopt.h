int chanopt_command(session *sess, char *tbuf, char *word[], char *word_eol[]);
bool chanopt_is_set(unsigned int global, unsigned char per_chan_setting);
bool chanopt_is_set_a(unsigned int global, unsigned char per_chan_setting);
void chanopt_save_all();
void chanopt_save(session *sess);
void chanopt_load(session *sess);
