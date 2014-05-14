-- modules

require "globals"

function createLibrary(longname,shortname)
	package = newpackage()
	package.path = project.path
	package.target = "lib" .. shortname

	package.name = longname
	package.language = "c"
	package.kind = "lib"

	package.libdir = "../../lib"
	package.bindir = "../../bin"
	package.objdir = "obj/" .. package.target

	table.insert(package.config['Release'].defines,"NDEBUG")

	package.defines = {
		"_CRT_SECURE_NO_WARNINGS"
		}

	if (OS == "windows") then
		table.insert(package.defines,"WIN32")
		table.insert(package.defines,"_WINDOWS")
	end

	package.files = {
	  matchfiles("../../lib/SRC/".. shortname .. "/*.c"),
	}

	package.includepaths = {
		"../../include",
	}

	package.excludes = {
		}

	-- package.libpaths = globals.osg.libpaths()


	if (OS == "windows") then
	end
	
	if (shortname ~= "AR") then
		package.links = { "libAR",}
	end

	package.config["Debug"].target = package.target .. globals.targetsuffix
	
	return package

end

function createVideoLibrary(longname,shortname)

	package = newpackage()
	package.path = project.path
	package.target = "lib" .. shortname

	package.name = longname
	package.language = "c++"
	package.kind = "dll"

	package.libdir = "../../lib"
	package.bindir = "../../bin"
	package.objdir = "obj/" .. package.target

	table.insert(package.config['Release'].defines,"NDEBUG")

	package.defines = {
		"_CRT_SECURE_NO_WARNINGS",
		"_USRDLL",
		"LIBARVIDEO_EXPORTS"
		}

	if (OS == "windows") then
		table.insert(package.defines,"WIN32")
		table.insert(package.defines,"_WINDOWS")
	end

	if (OS == "windows") then
		package.files = {
		  matchfiles("../../lib/SRC/VideoWin32DirectShow/*.cpp"),
		}
	end

	package.includepaths = {
		"../../include",
		"../../DSVL/src"
	}

	package.excludes = {
		}

	-- package.libpaths = globals.osg.libpaths()


	if (OS == "windows") then
	
		package.libpaths = { "../../DSVL/lib" }		
		package.config["Debug"].links = {"DSVLd"}
		package.config["Release"].links = {"DSVL"}
			
	end
	
	if (shortname ~= "AR") then
		package.links = { "libAR",}
	end

	package.config["Debug"].target = package.target .. globals.targetsuffix
	
	return package
end


function createExample(longname,shortname)

	package = newpackage()
	package.path = project.path
	package.target = shortname

	package.name = longname
	package.language = "c"
	package.kind = "dll"

	package.libdir = "../../lib"
	package.bindir = "../../bin"
	package.objdir = "obj/" .. package.target

	table.insert(package.config['Release'].defines,"NDEBUG")

	package.defines = {
		"_CRT_SECURE_NO_WARNINGS"
		}

	if (OS == "windows") then
		table.insert(package.defines,"WIN32")
		table.insert(package.defines,"_WINDOWS")
	end

	if (OS == "windows") then
		package.files = {
		  matchfiles("../../examples/shortName/*.c"),
		}
	end

	package.includepaths = {
		"../../include",
		"../../DSVL/src"
	}

	package.excludes = {
		}

	-- package.libpaths = globals.osg.libpaths()


	if (OS == "windows") then

	end
	
	if (shortname ~= "AR") then
		package.links = { "libAR","libARMulti", "libARvideo"}
	end

	package.config["Debug"].target = package.target .. globals.targetsuffix
	
	return package
end


