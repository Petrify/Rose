/**
 * @file logging.hpp
 * @author your name (you@domain.com)
 * @brief Declaration of loggers used in VE
 * @version 0.1
 * @date 2022-03-22
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <RoseLogging.hpp>
namespace loggers {
    Logger debug = getLogger("Render/Debug");;
    Logger engine = getLogger("Render/Engine");;
}
