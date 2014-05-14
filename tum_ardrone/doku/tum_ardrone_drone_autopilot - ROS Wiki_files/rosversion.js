// version.js, based on SeeSaw 1.0
function Version(sections) {
    var dotversion = ".version."

      // Tag shows unless already tagged
      $.each(sections.show, function(){$("span.version." + this).not(".versionshow,.versionhide").filter(".hidepart").addClass("versionshow").end().filter(".showpart").addClass("versionhide")})
      $.each(sections.show, function(){$("div" + dotversion + this).not(".versionshow,.versionhide").addClass("versionshow")})

      // Tag hides unless already tagged
      $.each(sections.hide, function(){$("span.version." + this).not(".versionshow,.versionhide").filter(".showpart").addClass("versionshow").end().filter(".hidepart").addClass("versionhide")})
      $.each(sections.hide, function(){$("div" + dotversion + this).not(".versionshow,.versionhide").addClass("versionhide")})

      // Show or hide according to tag
      $(".versionshow").removeClass("versionshow").filter("span").show().end().filter("div").show(0)
      $(".versionhide").removeClass("versionhide").filter("span").hide().end().filter("div").hide(0)
      }

function getURLParameter(name) {
    return decodeURIComponent((new RegExp('[?|&]' + name + '=' + '([^&;]+?)(&|#|;|$)').exec(location.search)||[,""])[1].replace(/\+/g, '%20'))||null;
}

function toggleDocStatus()
{
  if($("#doc_status").is(":hidden"))
  {
    $("#doc_status").slideDown();
  }
  else
  {
    $("#doc_status").slideUp();
  }
}

$(document).ready(function() {
    var activedistro = "groovy"; //CHANGE THIS LINE TO CHANGE THE DISTRO DISPLAYED BY DEFAULT
    var url_distro = getURLParameter('distro');
    if(url_distro)
    {
        activedistro=url_distro;
    }
    //$("div.version").not("."+activedistro).hide();
    $("div.version").hide();
    if($("#"+activedistro).length > 0)
    {
      $("#"+activedistro).click();//CHANGE THIS LINE TO THE DISTRO DISPLAYED BY DEFAULT
    }
    else
    {
      $("button:first").click();
    }
    $("input.version:hidden").each(function(){var bg = $(this).attr("value").split(":"); $("div.version." + bg[0]).css("background-color",bg[1]).removeClass(bg[0])});
})
