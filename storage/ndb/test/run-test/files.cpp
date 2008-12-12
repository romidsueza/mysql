#include "atrt.hpp"
#include <sys/types.h>
#include <dirent.h>

static bool create_directory(const char * path);

bool
setup_directories(atrt_config& config, int setup)
{
  /**
   * 0 = validate
   * 1 = setup
   * 2 = setup+clean
   */
  for (size_t i = 0; i < config.m_clusters.size(); i++)
  {
    atrt_cluster& cluster = *config.m_clusters[i];
    for (size_t j = 0; j<cluster.m_processes.size(); j++)
    {
      atrt_process& proc = *cluster.m_processes[j];
      const char * dir = proc.m_proc.m_cwd.c_str();
      struct stat sbuf;
      int exists = 0;
      if (lstat(dir, &sbuf) == 0)
      {
	if (S_ISDIR(sbuf.st_mode))
	  exists = 1;
	else
	  exists = -1;
      }
      
      switch(setup){
      case 0:
	switch(exists){
	case 0:
	  g_logger.error("Could not find directory: %s", dir);
	  return false;
	case -1:
	  g_logger.error("%s is not a directory!", dir);
	  return false;
	}
	break;
      case 1:
	if (exists == -1)
	{
	  g_logger.error("%s is not a directory!", dir);
	  return false;
	}
	break;
      case 2:
	if (exists == 1)
	{
	  if (!remove_dir(dir))
	  {
	    g_logger.error("Failed to remove %s!", dir);
	    return false;
	  }
	  exists = 0;
	  break;
	}
	else if (exists == -1)
	{
	  if (!unlink(dir))
	  {
	    g_logger.error("Failed to remove %s!", dir);
	    return false;
	  }
	  exists = 0;
	}
      }
      if (exists != 1)
      {
	if (!create_directory(dir))
	{
	  return false;
	}
      }
    }
  }
  return true;
}

static
void
printfile(FILE* out, Properties& props, const char * section, ...)
{
  Properties::Iterator it (&props);
  const char * name = it.first();
  if (name)
  {
    va_list ap;
    va_start(ap, section);
    /* const int ret = */ vfprintf(out, section, ap);
    va_end(ap);
    fprintf(out, "\n");
    
    for (; name; name = it.next())
    {
      const char* val;
      props.get(name, &val);
      fprintf(out, "%s %s\n", name + 2, val);
    }
    fprintf(out, "\n");
  }
  fflush(out);
}

bool
setup_files(atrt_config& config, int setup, int sshx)
{
  /**
   * 0 = validate
   * 1 = setup
   * 2 = setup+clean
   */
  BaseString mycnf;
  mycnf.assfmt("%s/my.cnf", g_basedir);
  
  if (!create_directory(g_basedir))
  {
    return false;
  }

  if (mycnf != g_my_cnf)
  {
    struct stat sbuf;
    int ret = lstat(mycnf.c_str(), &sbuf);
    
    if (ret == 0)
    {
      if (unlink(mycnf.c_str()) != 0)
      {
	g_logger.error("Failed to remove %s", mycnf.c_str());
	return false;
      }
    }
    
    BaseString cp = "cp ";
    cp.appfmt("%s %s", g_my_cnf, mycnf.c_str());
    if (system(cp.c_str()) != 0)
    {
      g_logger.error("Failed to '%s'", cp.c_str());
      return false;
    }
  }
  
  if (setup == 2 || config.m_generated)
  {
    /**
     * Do mysql_install_db
     */
    for (size_t i = 0; i < config.m_clusters.size(); i++)
    {
      atrt_cluster& cluster = *config.m_clusters[i];
      for (size_t j = 0; j<cluster.m_processes.size(); j++)
      {
	atrt_process& proc = *cluster.m_processes[j];
	if (proc.m_type == atrt_process::AP_MYSQLD)
	{
	  const char * val;
	  require(proc.m_options.m_loaded.get("--datadir=", &val));
	  BaseString tmp;
	  tmp.assfmt("%s/bin/mysql_install_db --defaults-file=%s/my.cnf --datadir=%s > /dev/null 2>&1",
		     g_prefix, g_basedir, val);
	  if (system(tmp.c_str()) != 0)
	  {
	    g_logger.error("Failed to mysql_install_db for %s, cmd: >%s<",
			   proc.m_proc.m_cwd.c_str(),
			   tmp.c_str());
	  }
	  else
	  {
	    g_logger.info("mysql_install_db for %s",
			  proc.m_proc.m_cwd.c_str());
	  }
	}
      }
    }
  }
  
  FILE * out = NULL;
  if (config.m_generated == false)
  {
    g_logger.info("Nothing configured...");
  }
  else
  {
    out = fopen(mycnf.c_str(), "a+");
    if (out == 0)
    {
      g_logger.error("Failed to open %s for append", mycnf.c_str());
      return false;
    }
    time_t now = time(0);
    fprintf(out, "#\n# Generated by atrt\n");
    fprintf(out, "# %s\n", ctime(&now));
  }
  
  for (size_t i = 0; i < config.m_clusters.size(); i++)
  {
    atrt_cluster& cluster = *config.m_clusters[i];
    if (out)
    {
      Properties::Iterator it(&cluster.m_options.m_generated);
      printfile(out, cluster.m_options.m_generated,
		"[mysql_cluster%s]", cluster.m_name.c_str());
    }
      
    for (size_t j = 0; j<cluster.m_processes.size(); j++)
    {
      atrt_process& proc = *cluster.m_processes[j];
      
      if (out)
      {
	switch(proc.m_type){
	case atrt_process::AP_NDB_MGMD:
	  printfile(out, proc.m_options.m_generated,
		    "[cluster_config.ndb_mgmd.%d%s]", 
		    proc.m_index, proc.m_cluster->m_name.c_str());
	  break;
	case atrt_process::AP_NDBD: 
	  printfile(out, proc.m_options.m_generated,
		    "[cluster_config.ndbd.%d%s]",
		    proc.m_index, proc.m_cluster->m_name.c_str());
	  break;
	case atrt_process::AP_MYSQLD:
	  printfile(out, proc.m_options.m_generated,
		    "[mysqld.%d%s]",
		    proc.m_index, proc.m_cluster->m_name.c_str());
	  break;
	case atrt_process::AP_NDB_API:
	  break;
	case atrt_process::AP_CLIENT:
	  printfile(out, proc.m_options.m_generated,
		    "[client.%d%s]",
		    proc.m_index, proc.m_cluster->m_name.c_str());
	  break;
	case atrt_process::AP_ALL:
	case atrt_process::AP_CLUSTER:
	  abort();
	}
      }
      
      /**
       * Create env.sh
       */
      BaseString tmp;
      tmp.assfmt("%s/env.sh", proc.m_proc.m_cwd.c_str());
      char **env = BaseString::argify(0, proc.m_proc.m_env.c_str());
      if (env[0] || proc.m_proc.m_path.length())
      {
	Vector<BaseString> keys;
	FILE *fenv = fopen(tmp.c_str(), "w+");
	if (fenv == 0)
	{
	  g_logger.error("Failed to open %s for writing", tmp.c_str());
	  return false;
	}
	for (size_t k = 0; env[k]; k++)
	{
	  tmp = env[k];
	  int pos = tmp.indexOf('=');
	  require(pos > 0);
	  env[k][pos] = 0;
	  fprintf(fenv, "%s=\"%s\"\n", env[k], env[k]+pos+1);
	  keys.push_back(env[k]);
	  free(env[k]);
	}
	if (proc.m_proc.m_path.length())
	{
	  fprintf(fenv, "CMD=\"%s", proc.m_proc.m_path.c_str());
	  if (proc.m_proc.m_args.length())
	  {
	    fprintf(fenv, " %s", proc.m_proc.m_args.c_str());
	  }
	  fprintf(fenv, "\"\nexport CMD\n");
	}
	
	fprintf(fenv, "PATH=%s/bin:%s/libexec:$PATH\n", g_prefix, g_prefix);
	keys.push_back("PATH");
	for (size_t k = 0; k<keys.size(); k++)
	  fprintf(fenv, "export %s\n", keys[k].c_str());
	fflush(fenv);
	fclose(fenv);
      }
      free(env);
      
      {
        tmp.assfmt("%s/ssh-login.sh", proc.m_proc.m_cwd.c_str());
        FILE* fenv = fopen(tmp.c_str(), "w+");
        if (fenv == 0)
        {
          g_logger.error("Failed to open %s for writing", tmp.c_str());
          return false;
        }
        fprintf(fenv, "#!/bin/sh\n");
        fprintf(fenv, "cd %s\n", proc.m_proc.m_cwd.c_str());
        fprintf(fenv, "[ -f /etc/profile ] && . /etc/profile\n");
        fprintf(fenv, ". env.sh\n");
        fprintf(fenv, "ulimit -Sc unlimited\n");
        fprintf(fenv, "bash -i");
        fflush(fenv);
        fclose(fenv);
      }
    }
  }
  
  if (out)
  {
    fflush(out);
    fclose(out);
  }

  return true;
}

static
bool
create_directory(const char * path)
{
  BaseString tmp(path);
  Vector<BaseString> list;
  if (tmp.split(list, "/") == 0)
  {
    g_logger.error("Failed to create directory: %s", tmp.c_str());
    return false;
  }
  
  BaseString cwd = "/";
  for (size_t i = 0; i < list.size(); i++) 
  {
    cwd.append(list[i].c_str());
    cwd.append("/");
    mkdir(cwd.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IXGRP | S_IRGRP);
  }

  struct stat sbuf;
  if (lstat(path, &sbuf) != 0 ||
      !S_ISDIR(sbuf.st_mode))
  {
    g_logger.error("Failed to create directory: %s (%s)", 
		   tmp.c_str(), 
		   cwd.c_str());
    return false;
  }
  
  return true;
}

bool
remove_dir(const char * path, bool inclusive)
{
  DIR* dirp = opendir(path);

  if (dirp == 0)
  {
    if(errno != ENOENT) 
    {
      g_logger.error("Failed to remove >%s< errno: %d %s", 
		     path, errno, strerror(errno));
      return false;
    }
    return true;
  }
  
  struct dirent * dp;
  BaseString name = path;
  name.append("/");
  while ((dp = readdir(dirp)) != NULL)
  {
    if ((strcmp(".", dp->d_name) != 0) && (strcmp("..", dp->d_name) != 0)) 
    {
      BaseString tmp = name;
      tmp.append(dp->d_name);

      if (remove(tmp.c_str()) == 0)
      {
	continue;
      }
      
      if (!remove_dir(tmp.c_str()))
      {
	closedir(dirp);
	return false;
      }
    }
  }

  closedir(dirp);
  if (inclusive)
  {
    if (rmdir(path) != 0)
    {
      g_logger.error("Failed to remove >%s< errno: %d %s", 
		     path, errno, strerror(errno));
      return false;
    }
  }
  return true;
}

