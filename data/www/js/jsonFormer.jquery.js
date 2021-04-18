/**
 * jQuery plugin that turn a json object into a bootstrap form
 * use:
 *  $('#form-container').jsonFormer({
 *      title: "Just point me at a div and pass me an object",
 *      jsonObject: JSON.parse(editor.getValue());
 *  });
 *
 *  Make changes in the form and then call
 *  var newObj = $('#form-container').jsonFormer('formData');
 *
 *  Or if you just want an object representing the changes:
 *  var diff = $('#form-container').jsonFormer('getDiff');
 *
 *  I made my own decisions about which bootstrap elements to use.  You can either
 *  modify the elements after the form is rendered or you can define some of your own
 *  templates like
 *  var diff = $('#form-container').jsonFormer('options', 'stringTemplate', '<input type="text"></input>);
 *
 *  To list all options: $('#form-container').jsonFormer('options');
 * @author John Snook
 *
 * Edited by Njaal Brekke for the ESP Ascom Alpaca Library
 * Fixed getDiff error due to 'prop' being overwritten by recursion
 * Change boolean to checkbox
 */
$.widget('jsnook.jsonFormer', {
    options: {
        title: 'The data you passed',
        rootTemplate: '<div id="json-object"><form class="form-horizontal json-form-inner" ></form></div>',
        objectTemplate: '<div id="json-object" class="card mb-2 mt-2"><div class="card-header"><div id="title"></div></div><div class="card-body"><form class="form-horizontal json-form-inner" ></form></div></div>',
        arrayTemplate: '<div id="json-array" class="form-group row"><label class="control-label col-sm-4 col-form-label"></label><form class="form-inline col-sm-8 json-form-inner"></form></div>',
        stringTemplate: '<div id="json-string" class="form-group row"><label for="" class="control-label col-sm-4 col-form-label"></label><div class="col-sm-8"><input type="text" class="form-control" value="" id="" placeholder="key"></div></div>',
        numberTemplate: '<div id="json-number" class="form-group row"><label for="" class="control-label col-sm-4 col-form-label"></label><div class="col-sm-8"><input type="text" class="form-control" value="" id="" placeholder="key"></div></div>',
        booleanTemplate: '<div id="json-boolean" class="form-group row"><label for="" class="control-label col-sm-4 col-form-label"></label><div class="col-sm-8"><div class="custom-control custom-switch"><input type="checkbox" class="custom-control-input"><label class="custom-control-label"></label></div></div></div>',
        nullTemplate: '<div id="json-null" class="form-group row"><label class="control-label col-sm-4 col-form-label" for=""></label><div class="col-sm-8"><p class="form-control-static">NULL</p></div></div>',
        emptyObjectTemplate: '<div id="json-null" class="form-group row"><label class="control-label col-sm-4 col-form-label" for=""></label><div class="col-sm-8"><p class="form-control-static">NULL</p></div></div>',
        inlineFieldTemplate: '<input type="text" class="form-control col-sm-4 " id="inline-field" placeholder="key">'
    },
    _create: function () {
        this.jsonObject = this.options.jsonObject;
        this._makeForm();
    },
    /**
     * Private function to render the form in our element from our data.
     * @returns jQuery
     */
    _makeForm: function () {
        var self = this;
        var pKey = 'form';
        var container = this.element;
        container.html('');
        var box = $(this.options.rootTemplate);
        $(container).append(box);
        container = $(box).find('form');
        self._recursiveFunction('data', this.jsonObject, container, pKey);
        $('[data-original]').on('keyup keypress blur change', function (e) {
            let val;
            if ($(e.target).attr('type') === 'checkbox') {
                val = $(e.target).is(':checked').toString();
            } else {
                val = $(e.target).val();
            }
            if (val !== $(e.target).attr('data-original')) {
                $(e.target).attr('data-changed', true);
            } else
                $(e.target).removeAttr('data-changed');
        });
    },
    /**
     * Redraws the form
     * @returns jQuery
     */
    refresh: function () {
        this._makeForm();
    },
    /**
     * Gets the changed items from our form and pokes them into a clone of
     * the original and return it
     *
     * @returns object {jsonFormAnonym$0@call;_objectDiff}
     */
    formData: function () {

        // cheap deep cloner
        var newObj = JSON.parse(JSON.stringify(this.jsonObject));

        changedElems = $('[data-changed="true"]');
        for (i = 0; i < changedElems.length; i++) {
            if (changedElems.hasOwnProperty(i)) {
                var id = $(changedElems[i]).attr('id');
                var val = $(changedElems[i]).val();
                var type = $(changedElems[i]).attr('type');
                var propertyChain = id.replaceAll('-', '.').replaceAll('form', '');
                switch (type) {
                    case "checkbox":
                        val = $(changedElems[i]).is(':checked').toString();
                        break;
                    case "number":
                        val *= 1;
                }
                /**
                 *  because we've stored the object address chain in id,
                 *  we can create an eval statment to change the property
                 *  in our clone
                 *
                 * @type String
                 */
                var statement = 'newObj' + propertyChain + ' = ' + (typeof(val) == "string"? '"' + val + '"' : val);
                eval(statement);
            }
        }
        return newObj;
    },
    getDiff: function () {
        return this._objectDiff(this.jsonObject, this.formData());
    },
    _objectDiff: function (oldObj, newObj, theDiff) {
        self = this;
        theDiff = {};
        $.each([oldObj, newObj], function (index, obj) {
            for (var prop in obj) {
                if (obj.hasOwnProperty(prop)) {
                    if (typeof obj[prop] === "object" && obj[prop] !== null) {
                        aDiff = self._objectDiff(oldObj[prop], newObj[prop], theDiff);
                        if ($.isEmptyObject(aDiff) === false)
                            theDiff[prop] = aDiff;
                    } else {
                        if (oldObj === undefined)
                            oldObj = {};
                        if (newObj === undefined)
                            newObj = {};
                        if (oldObj[prop] !== newObj[prop]) {
                            theDiff[prop] = newObj[prop];
                        }
                    }
                }
            }
        });
        return theDiff;
    },
    _recursiveFunction: function (key, val, container, parentKey) {
        $.each(val, this._eachClosure(container, parentKey));
    },
    _eachClosure: function (containerElem, parentKey) {
        var ppKey = parentKey;
        var self = this;
        var container = containerElem;
        return function (key, val) {
            var pKey = ppKey + (typeof (key) === 'number' ? '[' + key + ']' : '-' + key);
            var box;
            switch (typeof (val)) {
                case "array":
                    box = $(self.options.arrayTemplate);
                    $(box).find('label').text(key);
                    innerContainer = $(box).find('.json-form-inner');
                    $(container).append(box);
                    self._recursiveFunction(key, val, innerContainer, pKey);
                    break;
                case "object":
                    if (!self._isEmpty(val)) {
                        if ($.isArray(val)) {
                            box = $(self.options.arrayTemplate);
                            $(box).find('label').text(key);
                            innerContainer = $(box).find('.json-form-inner');
                        } else {
                            box = $(self.options.objectTemplate);
                            $(box).find('#title').html("<H5>"+key+"</H5>");
                            innerContainer = $(box).find('.json-form-inner');
                        }
                        $(container).append(box);
                        self._recursiveFunction(key, val, innerContainer, pKey);
                    } else {
                        box = $(self.options.emptyObjectTemplate);
                        val = JSON.stringify(val);
                        $(box).find('label').text(key);
                        $(box).find('p').attr('id', pKey)
                                .attr('value', val)
                                .attr('type', 'plain-text')
                                .attr('data-original', val)
                                .text(val);
                        $(container).append(box);
                    }
                    break;
                case "string":
                    if ($.isNumeric(key)) {
                        box = $(self.options.inlineFieldTemplate);
                    } else {
                        box = $(self.options.stringTemplate);
                        $(box).find('label').attr('for', pKey).text(key);
                        $(box).find('input')
                                .attr('id', pKey)
                                .attr('value', val)
                                .attr('data-original', val)
                                .attr('placeholder', key);
                    }
                    $(container).append(box);
                    break;
                case "number":
                    if ($.isNumeric(key)) {
                        box = $(self.options.inlineFieldTemplate);
                        $(box).attr('type', 'number')
                                .attr('id', pKey)
                                .attr('data-original', val)
                                .attr('value', val);
                    } else {
                        box = $(self.options.stringTemplate);
                        $(box).find('label').attr('for', pKey).text(key);
                        $(box).find('input')
                                .attr('id', pKey)
                                .attr('value', val)
                                .attr('placeholder', key)
                                .attr('data-original', val)
                                .attr('type', 'number')
                                .attr('step','any');
                    }
                    $(container).append(box);
                    break;
                case "boolean":
                    box = $(self.options.booleanTemplate);
                    $(box).find('label').first().text(key);
                    $(box).find('label').last().attr('for', pKey);
                    $(box).find('input')
                        .attr('id', pKey)
                        .attr('data-original', val)
                        .prop('checked', val);
                    $(container).append(box);
                    break;
                case "null":
                    box = $(self.options.nullTemplate);
                    $(box).find('label').attr('id', pKey).text(key);
                    $(box).find('p').attr('id', pKey)
                            .attr('value', JSON.stringify(val))
                            .attr('type', 'plain-text')
                            .text(JSON.stringify(val));
                    $(container).append(box);
                    break;
            }
        }
    },
    _isEmpty: function (obj) {
        for (var key in obj) {
            if (obj.hasOwnProperty(key))
                return false;
        }
        return true;
    }

});

String.prototype.replaceAll = function (search, replacement) {
    var target = this;
    return target.replace(new RegExp(search, 'g'), replacement);
};
