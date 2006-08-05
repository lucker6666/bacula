USE bacula;
--
-- Note, we use BLOB rather than TEXT because in MySQL,
--  BLOBs are identical to TEXT except that BLOB is case
--  sensitive in sorts, which is what we want, and TEXT
--  is case insensitive.
--
CREATE TABLE Filename (
  FilenameId INTEGER UNSIGNED NOT NULL AUTO_INCREMENT,
  Name BLOB NOT NULL,
  PRIMARY KEY(FilenameId),
  INDEX (Name(255))
  );

CREATE TABLE Path (
   PathId INTEGER UNSIGNED NOT NULL AUTO_INCREMENT,
   Path BLOB NOT NULL,
   PRIMARY KEY(PathId),
   INDEX (Path(255))
   );


CREATE TABLE File (
   FileId INTEGER UNSIGNED NOT NULL AUTO_INCREMENT,
   FileIndex INTEGER UNSIGNED NOT NULL DEFAULT 0,
   JobId INTEGER UNSIGNED NOT NULL REFERENCES Job,
   PathId INTEGER UNSIGNED NOT NULL REFERENCES Path,
   FilenameId INTEGER UNSIGNED NOT NULL REFERENCES Filename,
   MarkId INTEGER UNSIGNED NOT NULL DEFAULT 0,
   LStat TINYBLOB NOT NULL,
   MD5 TINYBLOB NOT NULL,
   PRIMARY KEY(FileId),
   INDEX (JobId),
   INDEX (JobId, PathId, FilenameId)
   );

#
# Possibly add one or more of the following indexes
#  to the above File table if your Verifies are
#  too slow.
#
#  INDEX (PathId),
#  INDEX (FilenameId),
#  INDEX (FilenameId, PathId)
#  INDEX (JobId),
#

CREATE TABLE MediaType (
   MediaTypeId INTEGER UNSIGNED NOT NULL AUTO_INCREMENT,
   MediaType TINYBLOB NOT NULL,
   ReadOnly TINYINT DEFAULT 0,
   PRIMARY KEY(MediaTypeId)
   );

CREATE TABLE Storage (
   StorageId INTEGER UNSIGNED NOT NULL AUTO_INCREMENT,
   Name TINYBLOB NOT NULL,
   AutoChanger TINYINT DEFAULT 0,
   PRIMARY KEY(StorageId)
   );

CREATE TABLE Device (
   DeviceId INTEGER UNSIGNED NOT NULL AUTO_INCREMENT,
   Name TINYBLOB NOT NULL,
   MediaTypeId INTEGER UNSIGNED NOT NULL REFERENCES MediaType,
   StorageId INTEGER UNSIGNED NOT NULL REFERENCES Storage,
   DevMounts INTEGER UNSIGNED DEFAULT 0,
   DevReadBytes BIGINT UNSIGNED DEFAULT 0,
   DevWriteBytes BIGINT UNSIGNED DEFAULT 0,
   DevReadBytesSinceCleaning BIGINT UNSIGNED DEFAULT 0,
   DevWriteBytesSinceCleaning BIGINT UNSIGNED DEFAULT 0,
   DevReadTime BIGINT UNSIGNED DEFAULT 0,
   DevWriteTime BIGINT UNSIGNED DEFAULT 0,
   DevReadTimeSinceCleaning BIGINT UNSIGNED DEFAULT 0,
   DevWriteTimeSinceCleaning BIGINT UNSIGNED DEFAULT 0,
   CleaningDate DATETIME DEFAULT 0,
   CleaningPeriod BIGINT UNSIGNED DEFAULT 0,
   PRIMARY KEY(DeviceId)
   );


CREATE TABLE Job (
   JobId INTEGER UNSIGNED NOT NULL AUTO_INCREMENT,
   Job TINYBLOB NOT NULL,
   Name TINYBLOB NOT NULL,
   Type BINARY(1) NOT NULL,
   Level BINARY(1) NOT NULL,
   ClientId INTEGER NULL REFERENCES Client,
   JobStatus BINARY(1) NOT NULL,
   SchedTime DATETIME NOT NULL,
   StartTime DATETIME NULL,
   EndTime DATETIME NULL,
   JobTDate BIGINT UNSIGNED NOT NULL,
   VolSessionId INTEGER UNSIGNED NOT NULL DEFAULT 0,
   VolSessionTime INTEGER UNSIGNED NOT NULL DEFAULT 0,
   JobFiles INTEGER UNSIGNED NOT NULL DEFAULT 0,
   JobBytes BIGINT UNSIGNED NOT NULL DEFAULT 0,
   JobErrors INTEGER UNSIGNED NOT NULL DEFAULT 0,
   JobMissingFiles INTEGER UNSIGNED NOT NULL DEFAULT 0,
   PoolId INTEGER UNSIGNED NULL REFERENCES Pool,
   FileSetId INTEGER UNSIGNED NULL REFERENCES FileSet,
   PurgedFiles TINYINT NOT NULL DEFAULT 0,
   HasBase TINYINT NOT NULL DEFAULT 0,
   PRIMARY KEY(JobId),
   INDEX (Name(128))
   );

CREATE TABLE MAC (
   JobId INTEGER UNSIGNED NOT NULL AUTO_INCREMENT,
   OriginalJobId INTEGER UNSIGNED NOT NULL,
   JobType BINARY(1) NOT NULL,
   JobLevel BINARY(1) NOT NULL,
   SchedTime DATETIME NOT NULL,
   StartTime DATETIME NOT NULL,
   EndTime DATETIME NOT NULL,
   JobTDate BIGINT UNSIGNED NOT NULL,
   PRIMARY KEY(JobId)
   );

CREATE TABLE Location (
   LocationId INTEGER UNSIGNED NOT NULL AUTO_INCREMENT,
   Location TINYBLOB NOT NULL,
   PRIMARY KEY(LocationId)
   );

# 
CREATE TABLE FileSet (
   FileSetId INTEGER UNSIGNED NOT NULL AUTO_INCREMENT,
   FileSet TINYBLOB NOT NULL,
   MD5 TINYBLOB NOT NULL,
   CreateTime DATETIME NOT NULL,
   PRIMARY KEY(FileSetId)
   );

CREATE TABLE JobMedia (
   JobMediaId INTEGER UNSIGNED NOT NULL AUTO_INCREMENT,
   JobId INTEGER UNSIGNED NOT NULL REFERENCES Job,
   MediaId INTEGER UNSIGNED NOT NULL REFERENCES Media,
   FirstIndex INTEGER UNSIGNED NOT NULL DEFAULT 0,
   LastIndex INTEGER UNSIGNED NOT NULL DEFAULT 0,
   StartFile INTEGER UNSIGNED NOT NULL DEFAULT 0,
   EndFile INTEGER UNSIGNED NOT NULL DEFAULT 0,
   StartBlock INTEGER UNSIGNED NOT NULL DEFAULT 0,
   EndBlock INTEGER UNSIGNED NOT NULL DEFAULT 0,
   VolIndex INTEGER UNSIGNED NOT NULL DEFAULT 0,
   Copy INTEGER UNSIGNED NOT NULL DEFAULT 0,
   Stripe INTEGER UNSIGNED NOT NULL DEFAULT 0,
   PRIMARY KEY(JobMediaId),
   INDEX (JobId, MediaId)
   );


CREATE TABLE Media (
   MediaId INTEGER UNSIGNED NOT NULL AUTO_INCREMENT,
   VolumeName TINYBLOB NOT NULL,
   Slot INTEGER NOT NULL DEFAULT 0,
   PoolId INTEGER UNSIGNED NOT NULL REFERENCES Pool,
   MediaType TINYBLOB NOT NULL,
   MediaTypeId INTEGER UNSIGNED NOT NULL REFERENCES MediaType,
   LabelType TINYINT NOT NULL DEFAULT 0,
   FirstWritten DATETIME NULL,
   LastWritten DATETIME NULL,
   LabelDate DATETIME NULL,
   VolJobs INTEGER UNSIGNED NOT NULL DEFAULT 0,
   VolFiles INTEGER UNSIGNED NOT NULL DEFAULT 0,
   VolBlocks INTEGER UNSIGNED NOT NULL DEFAULT 0,
   VolMounts INTEGER UNSIGNED NOT NULL DEFAULT 0,
   VolBytes BIGINT UNSIGNED NOT NULL DEFAULT 0,
   VolParts INTEGER UNSIGNED NOT NULL DEFAULT 0,
   VolErrors INTEGER UNSIGNED NOT NULL DEFAULT 0,
   VolWrites INTEGER UNSIGNED NOT NULL DEFAULT 0,
   VolCapacityBytes BIGINT UNSIGNED NOT NULL,
   VolStatus ENUM('Full', 'Archive', 'Append', 'Recycle', 'Purged',
    'Read-Only', 'Disabled', 'Error', 'Busy', 'Used', 'Cleaning') NOT NULL,
   Recycle TINYINT NOT NULL DEFAULT 0,
   VolRetention BIGINT UNSIGNED NOT NULL DEFAULT 0,
   VolUseDuration BIGINT UNSIGNED NOT NULL DEFAULT 0,
   MaxVolJobs INTEGER UNSIGNED NOT NULL DEFAULT 0,
   MaxVolFiles INTEGER UNSIGNED NOT NULL DEFAULT 0,
   MaxVolBytes BIGINT UNSIGNED NOT NULL DEFAULT 0,
   InChanger TINYINT NOT NULL DEFAULT 0,
   StorageId INTEGER UNSIGNED NOT NULL REFERENCES Storage,
   DeviceId INTEGER UNSIGNED NOT NULL REFERENCES Device,
   MediaAddressing TINYINT NOT NULL DEFAULT 0,
   VolReadTime BIGINT UNSIGNED NOT NULL DEFAULT 0,
   VolWriteTime BIGINT UNSIGNED NOT NULL DEFAULT 0,
   EndFile INTEGER UNSIGNED NOT NULL DEFAULT 0,
   EndBlock INTEGER UNSIGNED NOT NULL DEFAULT 0,
   LocationId INTEGER UNSIGNED NOT NULL REFERENCES Location,
   RecycleCount INTEGER UNSIGNED DEFAULT 0,
   InitialWrite DATETIME NULL,
   ScratchPoolId INTEGER UNSIGNED DEFAULT 0 REFERENCES Pool,
   RecyclePoolId INTEGER UNSIGNED DEFAULT 0 REFERENCES Pool,
   PRIMARY KEY(MediaId),
   INDEX (PoolId)
   );

CREATE INDEX inx8 ON Media (PoolId);



CREATE TABLE Pool (
   PoolId INTEGER UNSIGNED NOT NULL AUTO_INCREMENT,
   Name TINYBLOB NOT NULL,
   NumVols INTEGER UNSIGNED NOT NULL DEFAULT 0,
   MaxVols INTEGER UNSIGNED NOT NULL DEFAULT 0,
   UseOnce TINYINT NOT NULL,
   UseCatalog TINYINT NOT NULL,
   AcceptAnyVolume TINYINT DEFAULT 0,
   VolRetention BIGINT UNSIGNED NOT NULL,
   VolUseDuration BIGINT UNSIGNED NOT NULL,
   MaxVolJobs INTEGER UNSIGNED NOT NULL DEFAULT 0,
   MaxVolFiles INTEGER UNSIGNED NOT NULL DEFAULT 0,
   MaxVolBytes BIGINT UNSIGNED NOT NULL,
   AutoPrune TINYINT DEFAULT 0,
   Recycle TINYINT DEFAULT 0,
   PoolType ENUM('Backup', 'Copy', 'Cloned', 'Archive', 'Migration', 'Scratch') NOT NULL,
   LabelType TINYINT NOT NULL DEFAULT 0,
   LabelFormat TINYBLOB,
   Enabled TINYINT DEFAULT 1,
   ScratchPoolId INTEGER UNSIGNED DEFAULT 0 REFERENCES Pool,
   RecyclePoolId INTEGER UNSIGNED DEFAULT 0 REFERENCES Pool,
   NextPoolId INTEGER UNSIGNED DEFAULT 0 REFERENCES Pool,
   MigrationHighBytes BIGINT UNSIGNED DEFAULT 0,
   MigrationLowBytes BIGINT UNSIGNED DEFAULT 0,
   MigrationTime BIGINT UNSIGNED DEFAULT 0,
   UNIQUE (Name(128)),
   PRIMARY KEY (PoolId)
   );


CREATE TABLE Client (
   ClientId INTEGER UNSIGNED NOT NULL AUTO_INCREMENT,
   Name TINYBLOB NOT NULL,
   Uname TINYBLOB NOT NULL,	  /* full uname -a of client */
   AutoPrune TINYINT DEFAULT 0,
   FileRetention BIGINT UNSIGNED NOT NULL,
   JobRetention  BIGINT UNSIGNED NOT NULL,
   UNIQUE (Name(128)),
   PRIMARY KEY(ClientId)
   );

CREATE TABLE BaseFiles (
   BaseId INTEGER UNSIGNED AUTO_INCREMENT,
   BaseJobId INTEGER UNSIGNED NOT NULL REFERENCES Job,
   JobId INTEGER UNSIGNED NOT NULL REFERENCES Job,
   FileId INTEGER UNSIGNED NOT NULL REFERENCES File,
   FileIndex INTEGER UNSIGNED,
   PRIMARY KEY(BaseId)
   );

CREATE TABLE UnsavedFiles (
   UnsavedId INTEGER UNSIGNED AUTO_INCREMENT,
   JobId INTEGER UNSIGNED NOT NULL REFERENCES Job,
   PathId INTEGER UNSIGNED NOT NULL REFERENCES Path,
   FilenameId INTEGER UNSIGNED NOT NULL REFERENCES Filename,
   PRIMARY KEY (UnsavedId)
   );



CREATE TABLE Counters (
   Counter TINYBLOB NOT NULL,
   MinValue INTEGER,
   MaxValue INTEGER,
   CurrentValue INTEGER,
   WrapCounter TINYBLOB NOT NULL,
   PRIMARY KEY (Counter(128))
   );

CREATE TABLE CDImages (
   MediaId INTEGER UNSIGNED NOT NULL,
   LastBurn DATETIME NOT NULL,
   PRIMARY KEY (MediaId)
   );

CREATE TABLE Status (
   JobStatus CHAR(1) BINARY NOT NULL,
   JobStatusLong BLOB, 
   PRIMARY KEY (JobStatus)
   );

INSERT INTO Status (JobStatus,JobStatusLong) VALUES
   ('C', 'Created, not yet running'),
   ('R', 'Running'),
   ('B', 'Blocked'),
   ('T', 'Completed successfully'),
   ('E', 'Terminated with errors'),
   ('e', 'Non-fatal error'),
   ('f', 'Fatal error'),
   ('D', 'Verify found differences'),
   ('A', 'Canceled by user'),
   ('F', 'Waiting for Client'),
   ('S', 'Waiting for Storage daemon'),
   ('m', 'Waiting for new media'),
   ('M', 'Waiting for media mount'),
   ('s', 'Waiting for storage resource'),
   ('j', 'Waiting for job resource'),
   ('c', 'Waiting for client resource'),
   ('d', 'Waiting on maximum jobs'),
   ('t', 'Waiting on start time'),
   ('p', 'Waiting on higher priority jobs');

CREATE TABLE Version (
   VersionId INTEGER UNSIGNED NOT NULL 
   );

-- Initialize Version		 
INSERT INTO Version (VersionId) VALUES (9);
