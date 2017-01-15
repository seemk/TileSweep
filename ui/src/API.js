import $ from "jquery";

export function startPrerender(prerender, success, fail) {
  return $.ajax({
    type: "POST",
    url: "/prerender",
    dataType: "json",
    contentType: "application/json",
    data: JSON.stringify(prerender),
    success: success,
    error: fail
  });
}
