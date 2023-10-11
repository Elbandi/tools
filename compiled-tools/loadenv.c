
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <err.h>
#include <string.h>
#include <tool/libarray.h>

static char* helptext =
"Usage: loadenv [-0c] <FILE> <COMMAND> [<ARG> [...]]\n"
"See loadenv(1).\n"
;


struct dedupdata {
	size_t envnamelen;
	char * env;
	Array** envarray;
};

array_loop_control
remove_duplicate(array_index_t index, char* item, struct dedupdata * dedupdata)
{
	if(strchr(item, '=')-item == dedupdata->envnamelen && strncmp(item, dedupdata->env, dedupdata->envnamelen) == 0)
	{
		array_delete(dedupdata->envarray, index, 1);
		return ARRAY_LOOP_STOP;
	}
	return ARRAY_LOOP_CONTINUE;
}

#ifdef DEBUG
array_loop_control
pprint(array_index_t index, char * item, void * x)
{
	warnx("index %d item %s", index, item);
	return ARRAY_LOOP_CONTINUE;
}
#endif

int main(int argc, char** argv, char** current_env)
{
	FILE * envfile;
	Array* envarray;
	unsigned char argidx;
	unsigned char delimiter;
	char * env;
	size_t envsize;
	size_t envnamelen;
	ssize_t read_status;
	char * envval;
	struct dedupdata dedupdata;
	unsigned char copy_current_environment;
	unsigned int it;
	
	delimiter = '\n';
	copy_current_environment = 0;
	argidx = 1;
	
	while(argidx < argc)
	{
		if(strcmp(argv[argidx], "-0")==0)
		{
			delimiter = '\0';
		}
		else if(strcmp(argv[argidx], "-c")==0)
		{
			copy_current_environment = 1;
		}
		else if(strcmp(argv[argidx], "--help")==0)
		{
			printf("%s", helptext);
			exit(0);
		}
		else if(argv[argidx][0] == '-')
		{
			errx(-2, "unknown option: %s", argv[argidx]);
		}
		else {
			break;
		}
		argidx++;
	}
	
	if(argidx+1 < argc)
	{
		if(envfile = fopen(argv[argidx], "r"))
		{
			array_init(&envarray, 2);
			
			/* copy current environment */
			if(copy_current_environment)
			{
				for(it = 0; current_env[it] != NULL; it++)
				{
					array_append(&envarray, current_env[it]);
				}
			}
			
			/* read up environments from file */
			while(1)
			{
				env = NULL;
				envsize = 0;
				read_status = getdelim(&env, &envsize, delimiter, envfile);
				if(read_status == -1) break;
				if(!env) goto read_next_env;
				
				// chomp
				envsize = strlen(env);
				if(envsize == 0) goto read_next_env;
				if(env[envsize-1] == delimiter) env[envsize-1] = '\0';
				envsize = strlen(env);
				
				// ignore empty
				if(envsize == 0) goto read_next_env;
				
				// ignore comments
				if(delimiter == '\n' && env[0] == '#') goto read_next_env;
				
				// locate value
				envval = strchr(env, '=');
				
				if(envval == NULL)
				{
					// remove the named environment variable if it's a name only without '=' equal sign in it.
					dedupdata.envnamelen = envsize;
					dedupdata.env = env;
					dedupdata.envarray = &envarray;
					array_foreach(&envarray, 0, (array_loop_control(*)(array_index_t, char*, void*))remove_duplicate, &dedupdata);
				}
				else
				{
					envval++;  // envval point past to the '=' equal sign
					
					#ifdef DEBUG
					array_foreach(&envarray, 0, pprint, NULL);
					#endif
					
					// deduplicate
					dedupdata.envnamelen = envval - env - 1;
					dedupdata.env = env;
					dedupdata.envarray = &envarray;
					array_foreach(&envarray, 0, (array_loop_control(*)(array_index_t, char*, void*))remove_duplicate, &dedupdata);
					
					// add to the env list
					array_append(&envarray, env);
				}
				
				read_next_env:
				if(env) free(env);
			}
			
			array_append(&envarray, NULL);
		}
		else
		{
			perror("fopen");
			goto fail;
		}
		
		#ifdef DEBUG
		array_foreach(&envarray, 0, pprint, NULL);
		#endif
		
		execvpe(argv[argidx+1], &argv[argidx+1], array_getarray(&envarray));
		goto fail;
	}
	else
	{
		fprintf(stderr, "%s", helptext);
		exit(-2);
	}
	
	fail:
	return errno == 0 ? -1 : errno;
}
