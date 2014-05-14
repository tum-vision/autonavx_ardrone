--
-- premake script to create various versions of project files
--

require "globals"
require "modules"

project.name = "ARToolKit"
project.configs = { "Debug", "Release" }

if     (target == "vs2003") then
	project.path = "../VisualStudio/VS2003"
elseif (target == "vs2005") then
	project.path = "../VisualStudio/VS2005"
end

--
-- ARToolKit
--
createLibrary("Core Library","AR")
package = createLibrary("MultiMarker Library","ARmulti")
package = createVideoLibrary("Video Library","ARvideo")
-- package = createVideoLibrary("Graphics Library (Light)","gsub_lite")
-- package = createGraphicsLibrary("Graphics Library (GLUT)","gsub_lite")
